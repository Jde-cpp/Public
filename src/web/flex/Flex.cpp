#include <jde/web/flex/Flex.h>
#include <jde/crypto/OpenSsl.h>
//#include <jde/crypto/CryptoSettings.h>
#include "CancellationSignals.h"


namespace Jde::Web{
	static sp<LogTag> _logTag = Logging::Tag( "web" );
	static sp<LogTag> _requestTag = Logging::Tag( "web.request" );
	static sp<LogTag> _responseTag = Logging::Tag( "web.response" );
	α Flex::WebTag()ι->sp<LogTag>{ return _logTag; }
	α Flex::RequestTag()ι->sp<LogTag>{ return _requestTag; }
	α Flex::ResponseTag()ι->sp<LogTag>{ return _responseTag; }
	Flex::CancellationSignals _cancellationSignals;

	sp<Flex::IRequestHandler> _requestHandler;
	α Flex::GetRequestHandler()ι->sp<IRequestHandler>{ return _requestHandler; }

	namespace Flex{
		α LoadServerCertificate( ssl::context& ctx )->void;
		α Internal::HandleCustomRequest( HttpRequest req, sp<RestStream> stream )ι->HttpTask{
			if( auto p = GetRequestHandler(); p ){
				try{
					auto result = co_await *( p->HandleRequest(move(req)) );
					if( !result.Request ) THROW( "Request not set." );
					Send( move(*result.Request), move(stream), move(result.Json) );
				}
				catch( IRestException& e ){
					Send( move(e), move(stream) );
				}
				catch( IException& e ){
					e.SetLevel( ELogLevel::Critical );//no request object...
					Send( RestException<http::status::internal_server_error>(SRCE_CUR, move(req), move(e), "Error handling request."), move(stream) );
				}
			}
			else
				Send( RestException<http::status::not_implemented>(SRCE_CUR, move(req), "No handler set."), move(stream) );
			co_return;
		}
	}

	α Flex::Fail( beast::error_code ec, char const* what )ι->void{
		auto level{ ELogLevel::Error };
		switch( ec.value() ){
			case net::error::operation_aborted: //EOF
				level = ELogLevel::Debug;
			break;
			case net::ssl::error::stream_truncated: //also known as an SSL "short read", peer closed the connection without performing the required closing handshake.
			case 0xA000416: //ERR_SSL_SSLV3_ALERT_CERTIFICATE_UNKNOWN: The client doesn't trust the certificate.
				level = ELogLevel::Trace;
			break;
		}
		CodeException{ static_cast<std::error_code>(ec), level };
	}

	bool _started{};
	α Flex::HasStarted()ι->bool{ return _started; }

	sp<Flex::net::io_context> _ioc;
	α Flex::GetIOContext()ι->sp<net::io_context>{ return _ioc; }

	//PortType port, sv address={}, uint8 threadCount=1
	α Flex::Start( sp<IRequestHandler> pHandler )ε->void{
		_requestHandler = pHandler;
		ssl::context ctx{ ssl::context::tlsv12 };
		LoadServerCertificate( ctx );
		var port = Settings::Get<PortType>( "http/port" ).value_or( 6809 );
		var threadCount = Settings::Get<uint8>( "http/threads" ).value_or( 1 );
		_ioc = ms<net::io_context>( threadCount );
    net::co_spawn( *_ioc, Listen(ctx, tcp::endpoint{net::ip::make_address(Settings::Get<string>("http/address").value_or("0.0.0.0")), port}), net::bind_cancellation_slot(_cancellationSignals.slot(), net::detached) );

/*    net::signal_set signals( *_ioc, SIGINT, SIGTERM );//TODO Shutdown routine
    signals.async_wait( [&]( const beast::error_code&, int sig ){
     	if( sig == SIGINT )
				cancellation.emit( net::cancellation_type::all );
			else
				_ioc->stop(); // Stop the `io_context`. This will cause `run()` to return immediately, eventually destroying the `io_context` and all of the sockets in it.
    });
*/
    std::vector<std::jthread> v; // Run the I/O service on the requested number of threadCount
    v.reserve( threadCount - 1 );
    for( auto i = threadCount - 1; i > 0; --i ){
      v.emplace_back( [=]{ Threading::SetThreadDscrptn( Jde::format("Beast[{}]", i) ); _ioc->run(); } );
		}
		_started = true;
		Threading::SetThreadDscrptn( "Beast[0]" );
    _ioc->run();
		_started = false;
    for( auto& t : v )// (If we get here, it means we got a SIGINT or SIGTERM)
      t.join();
	}

	α Flex::Stop( bool terminate )ι->void{
		if( terminate )
			_ioc->stop(); // Stop the `io_context`. This will cause `run()` to return immediately, eventually destroying the `io_context` and all of the sockets in it.
		else
			_cancellationSignals.emit( net::cancellation_type::all );
	}

	α Flex::Listen( ssl::context& ctx, tcp::endpoint endpoint )ι->net::awaitable<void, executor_type>{
    typename tcp::acceptor::rebind_executor<executor_with_default>::other acceptor{ co_await net::this_coro::executor };
    if( !InitListener(acceptor, endpoint) )
      co_return;

    while( (co_await net::this_coro::cancellation_state).cancelled() == net::cancellation_type::none ){
			auto [ec, sock] = co_await acceptor.async_accept();
			const auto exec = sock.get_executor();
			var userEndpoint = sock.remote_endpoint();
			if( !ec )
				net::co_spawn( exec, DetectSession(StreamType(move(sock)), ctx, move(userEndpoint)), net::bind_cancellation_slot(_cancellationSignals.slot(), net::detached) );// We dont't need a strand, since the awaitable is an implicit strand.
    }
	}

	α Flex::InitListener( typename tcp::acceptor::rebind_executor<executor_with_default>::other& acceptor, const tcp::endpoint& endpoint )ι->bool{
    beast::error_code ec;
    acceptor.open( endpoint.protocol(), ec );
    if( ec ){
			Fail( ec, "open" );
			return false;
    }

    acceptor.set_option( net::socket_base::reuse_address(true), ec );// Allow address reuse
    if( ec ){
			Fail( ec, "set_option" );
			return false;
    }

    acceptor.bind( endpoint, ec );// Bind to the server address
    if( ec ){
			Fail( ec, "bind" );
			return false;
    }
    acceptor.listen( net::socket_base::max_listen_connections, ec );
    if( ec ){
			Fail( ec, "listen" );
			return false;
    }
    return true;
	}

	α Flex::DetectSession( StreamType stream, net::ssl::context& ctx, tcp::endpoint userEndpoint )ι->net::awaitable<void, executor_type>{
    beast::flat_buffer buffer;
    stream.expires_after( std::chrono::seconds(30) );// Set the timeout.
    auto [ec, result] = co_await beast::async_detect_ssl( stream, buffer );// on_run
    if( ec )
			co_return Fail(ec, "detect");
    if( result ){
			beast::ssl_stream<StreamType> ssl_stream{ move(stream), ctx };
			auto [ec, bytes_used] = co_await ssl_stream.async_handshake( net::ssl::stream_base::server, buffer.data() );
			if(ec)
				co_return Fail( ec, "handshake" );

			buffer.consume(bytes_used);
			co_await RunSession( ssl_stream, buffer, move(userEndpoint), true );
    }
    else
			co_await RunSession( stream, buffer, move(userEndpoint), false );
	}
namespace Flex{
	α GraphQL( HttpRequest req, sp<RestStream> stream )->Task{
		try{
			auto& query = req["query"]; THROW_IFX( query.empty(), RestException<http::status::bad_request>(SRCE_CUR, move(req), "No query sent.") );
			var sessionId = req.SessionInfo.SessionId;
			TRACET( RequestTag(), "[{:x}] - {}", sessionId, query );
			string threadDesc = Jde::format( "[{:x}]{}", sessionId, req.Target() );
			var y = await( json, DB::CoQuery(move(query), req.UserPK(), threadDesc) );
			TRACET( ResponseTag(), "[{:x}] - {}", sessionId, y.dump() );
			Send( move(req), move(stream), move(y) );
		}
		catch( IRestException& e ){
			Send( move(e), move(stream) );
			co_return;
		}
		catch( IException& e ){
			Send( RestException(SRCE_CUR, move(req), move(e), "Query failed."), move(stream) );
			co_return;
		}
	}
	α Internal::HandleRequest( HttpRequest req, sp<RestStream> stream )ι->Sessions::UpsertAwait::Task{
		try{
			req.SessionInfo = co_await Sessions::UpsertAwait( req.Header("authorization"), req.UserEndpoint, false );
		}
		catch( IException& e ){
			Send( RestException<http::status::unauthorized>(SRCE_CUR, move(req), move(e), "Could not get sessionInfo."), move(stream) );
			co_return;
		}
		if( req.Method() == http::verb::get && req.Target()=="/graphql" ){
			GraphQL( move(req), stream );
		}
		else
			HandleCustomRequest( move(req), move(stream) );
		co_return;
	}

	α Internal::SendOptions( const HttpRequest&& req )ι->http::message_generator{
		auto res = req.Response<http::empty_body>( http::status::no_content );
		res.set( http::field::access_control_allow_methods, "GET, POST, OPTIONS" );
		res.set( http::field::access_control_allow_headers, "*" );
		res.set( http::field::access_control_max_age, "7200" ); //2 hours chrome max
		return res;
	}

}
	α Flex::Send( HttpRequest&& req, sp<RestStream> stream, json j )ι->void{
		auto res = req.Response( move(j) );
		stream->AsyncWrite( move(res) );
	}

	α Flex::Send( IRestException&& e, sp<RestStream> stream )ι->void{
		auto res = e.Response();
		stream->AsyncWrite( move(res) );
	}


	α Flex::LoadServerCertificate( ssl::context& ctx )->void{
		Crypto::CryptoSettings settings{ "http/ssl" };//Linux - /etc/ssl/certs/server.crt and /etc/ssl/private/server.key
		if( !fs::exists(settings.PrivateKeyPath) ){
			settings.CreateDirectories();
			Crypto::CreateKeyCertificate( settings );
		}
    ctx.set_options( boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use );
		var cert = IO::FileUtilities::Load( settings.CertPath );
    ctx.use_certificate_chain( boost::asio::buffer(cert.data(), cert.size()) );

		ctx.set_password_callback( [=](uint, boost::asio::ssl::context_base::password_purpose){ return settings.Passcode; } );
		var key = IO::FileUtilities::Load( settings.PrivateKeyPath );
    ctx.use_private_key( boost::asio::buffer(key.data(), key.size()), boost::asio::ssl::context::file_format::pem );
    static const string dhStatic =
        "-----BEGIN DH PARAMETERS-----\n"
        "MIIBCAKCAQEArzQc5mpm0Fs8yahDeySj31JZlwEphUdZ9StM2D8+Fo7TMduGtSi+\n"
        "/HRWVwHcTFAgrxVdm+dl474mOUqqaz4MpzIb6+6OVfWHbQJmXPepZKyu4LgUPvY/\n"
        "4q3/iDMjIS0fLOu/bLuObwU5ccZmDgfhmz1GanRlTQOiYRty3FiOATWZBRh6uv4u\n"
        "tff4A9Bm3V9tLx9S6djq31w31Gl7OQhryodW28kc16t9TvO1BzcV3HjRPwpe701X\n"
        "oEEZdnZWANkkpR/m/pfgdmGPU66S2sXMHgsliViQWpDCYeehrvFRHEdR9NV+XJfC\n"
        "QMUk26jPTIVTLfXmmwU0u8vUkpR7LQKkwwIBAg==\n"
        "-----END DH PARAMETERS-----\n";
		string dh = fs::exists( settings.DhPath ) ? IO::FileUtilities::Load( settings.DhPath ) : dhStatic;
    ctx.use_tmp_dh( boost::asio::buffer(dh.data(), dh.size()) );
	}
}