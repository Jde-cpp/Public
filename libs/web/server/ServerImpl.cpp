#include "ServerImpl.h"
#include <jde/crypto/OpenSsl.h>
//#include <jde/web/server/Server.h>
#include <jde/web/server/IApplicationServer.h>
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/web/server/IRequestHandler.h>
#include <jde/web/server/RestException.h>
#include <jde/ql/ql.h>
#include <jde/framework/thread/execution.h>
#define let const auto

namespace Jde::Web{
	up<Server::IApplicationServer> _appServer;
	α Server::AppGraphQLAwait( string&& q, UserPK userPK, SL sl )ι->up<TAwait<jobject>>{ return _appServer->GraphQL( move(q), userPK, sl ); }
	α Server::SessionInfoAwait( SessionPK sessionPK, SL sl )ι->up<TAwait<App::Proto::FromServer::SessionInfo>>{ return _appServer->SessionInfoAwait( sessionPK, sl ); }

	sp<net::cancellation_signal> _cancelSignal;
	optional<ssl::context> _ctx;

	up<Server::IRequestHandler> _requestHandler;
	α Server::GetRequestHandler()ι->IRequestHandler&{ return *_requestHandler; }

	atomic_flag _started{};

namespace Server{
	atomic<uint32> _sequence;
	α DetectSession( StreamType stream, tcp::endpoint userEndpoint, sp<net::cancellation_signal> cancel )ι->net::awaitable<void, executor_type>{
		beast::flat_buffer buffer;
		stream.expires_after( std::chrono::seconds(30) );// Set the timeout.
		auto [ec, result] = co_await beast::async_detect_ssl( stream, buffer );// on_run
		if( ec ){
			CodeException{ ec, ELogTags::Server | ELogTags::Http, ELogLevel::Warning };
			co_return;
		}
		uint32 index = ++_sequence;
		if( result ){
			beast::ssl_stream<StreamType> ssl_stream{ move(stream), *_ctx };
			auto [ec, bytes_used] = co_await ssl_stream.async_handshake( ssl::stream_base::server, buffer.data() );
			if(ec){
				CodeException{ ec, ELogTags::Server | ELogTags::Http, ELogLevel::Warning };
				co_return;
			}

			buffer.consume(bytes_used);
			co_await RunSession( ssl_stream, buffer, move(userEndpoint), true, index, cancel );
		}
		else
			co_await RunSession( stream, buffer, move(userEndpoint), false, index, cancel );
	}

	α Send( HttpRequest&& req, sp<RestStream> stream, jvalue j, SRCE )ι->void{
		auto res = req.Response( move(j), sl );
		stream->AsyncWrite( move(res) );
	}

	α Send( IRestException&& e, sp<RestStream> stream )ι->void{
		auto res = e.Response();
		stream->AsyncWrite( move(res) );
	}

	α GraphQL( HttpRequest req, sp<RestStream> stream )->QL::QLAwait::Task{
		try{
			auto& query = req["query"]; THROW_IFX( query.empty(), RestException<http::status::bad_request>(SRCE_CUR, move(req), "No query sent.") );
			let sessionId = req.SessionInfo->SessionId;
			req.LogRead( query );
			string threadDesc = Jde::format( "[{:x}]{}", sessionId, req.Target() );
			auto y = co_await QL::QLAwait{move(query), req.UserPK() };

			Send( move(req), move(stream), move(y) );
		}
		catch( IRestException& e ){
			Send( move(e), move(stream) );
			co_return;
		}
		catch( IException& e ){
			if( !empty(e.Tags() & ELogTags::Parsing) )
				Send( RestException<http::status::bad_request>{move(e), move(req), "Query parsing failed."}, move(stream) );
			else
				Send( RestException{move(e), move(req), "Query failed."}, move(stream) );
			co_return;
		}
	}

	α HandleCustomRequest( HttpRequest req, sp<RestStream> stream )ι->IHttpRequestAwait::Task{
		try{
			HttpTaskResult result = co_await *( GetRequestHandler().HandleRequest(move(req)) );
			if( !result.Request ) THROW( "Request not set." );
			Send( move(*result.Request), move(stream), move(result.Json) );
		}
		catch( IRestException& e ){
			Send( move(e), move(stream) );
		}
		catch( IException& e ){
			e.SetLevel( ELogLevel::Critical );//no request object...
			Send( RestException<>{move(e), move(req), "Error handling request."}, move(stream) );
		}
	}

	α InitListener( typename tcp::acceptor::rebind_executor<executor_with_default>::other& acceptor, const tcp::endpoint& endpoint )ι->bool{
		beast::error_code ec;
		acceptor.open( endpoint.protocol(), ec );
		if( ec ){
			CodeException{ ec, ELogTags::Server | ELogTags::Http, ELogLevel::Critical };
			return false;
		}

		acceptor.set_option( net::socket_base::reuse_address(true), ec );// Allow address reuse
		if( ec ){
			CodeException{ ec, ELogTags::Server | ELogTags::Http, ELogLevel::Critical };
			return false;
		}

		acceptor.bind( endpoint, ec );// Bind to the server address
		if( ec ){
			CodeException{ ec, ELogTags::Server | ELogTags::Http, ELogLevel::Critical };
			return false;
		}
		acceptor.listen( net::socket_base::max_listen_connections, ec );
		if( ec ){
			CodeException{ ec, ELogTags::Server | ELogTags::Http, ELogLevel::Critical };
			return false;
		}
		return true;
	}

	α LoadServerCertificate()ε->void{
		_ctx = ssl::context{ ssl::context::tlsv12 };
		Crypto::CryptoSettings settings{ "http/ssl" };//Linux - /etc/ssl/certs/server.crt and /etc/ssl/private/server.key
		if( !fs::exists(settings.PrivateKeyPath) ){
			settings.CreateDirectories();
			Crypto::CreateKeyCertificate( settings );
		}
		_ctx->set_options( ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::single_dh_use );
		let cert = IO::FileUtilities::Load( settings.CertPath );
		_ctx->use_certificate_chain( net::buffer(cert.data(), cert.size()) );

		_ctx->set_password_callback( [=](uint, ssl::context_base::password_purpose){ return settings.Passcode; } );
		let key = IO::FileUtilities::Load( settings.PrivateKeyPath );
		_ctx->use_private_key( net::buffer(key.data(), key.size()), ssl::context::file_format::pem );
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
		_ctx->use_tmp_dh( net::buffer(dh.data(), dh.size()) );
	}

	α Listen( tcp::endpoint endpoint )ι->net::awaitable<void, executor_type>{
		typename tcp::acceptor::rebind_executor<executor_with_default>::other acceptor{ co_await net::this_coro::executor };
		if( !InitListener(acceptor, endpoint) )
			co_return;

		Trace( ELogTags::App, "Web Server accepting." );
		_started.test_and_set();
		_started.notify_all();
		while( (co_await net::this_coro::cancellation_state).cancelled() == net::cancellation_type::none ){
			auto [ec, sock] = co_await acceptor.async_accept();
			const auto exec = sock.get_executor();
			let userEndpoint = sock.remote_endpoint();
			if( !ec ){
				auto cancelSignal = ms<net::cancellation_signal>();
				net::co_spawn( exec, DetectSession(StreamType(move(sock)), move(userEndpoint), cancelSignal), net::bind_cancellation_slot(cancelSignal->slot(), net::detached) );// We dont't need a strand, since the awaitable is an implicit strand.
				Execution::AddCancelSignal( cancelSignal );
			}
		}
	}

	α Internal::Start( up<IRequestHandler>&& handler, up<IApplicationServer>&& server )ε->void{
		_requestHandler = move( handler );
		_appServer = move( server );
		if( !_ctx )
			LoadServerCertificate();

		let port = Settings::FindNumber<PortType>( "http/port" ).value_or( 6809 );

		_cancelSignal = ms<net::cancellation_signal>();
		auto address = tcp::endpoint{ net::ip::make_address(Settings::FindSV("http/address").value_or("0.0.0.0")), port };
		net::co_spawn( *Executor(), Listen(address), net::bind_cancellation_slot(_cancelSignal->slot(), net::detached) );
		Execution::AddCancelSignal( _cancelSignal );
		Execution::Run();
		_started.wait( false );
		Information( ELogTags::App, "Web Server started:  {}:{}.", address.address().to_string(), address.port() );
	}

	α Internal::Stop( bool /*terminate*/ )ι->void{
		ASSERT( _cancelSignal );
		_started.clear();
		_started.notify_all();
		if( _cancelSignal )
			_cancelSignal->emit( net::cancellation_type::all );
		_cancelSignal = nullptr;
		_ctx.reset();
	}
}
	α Server::HandleRequest( HttpRequest req, sp<RestStream> stream )ι->Sessions::UpsertAwait::Task{
		try{
			req.SessionInfo = co_await Sessions::UpsertAwait( req.Header("authorization"), req.UserEndpoint.address().to_string(), false );
		}
		catch( IException& e ){
			//Add error code.
			//capture error code on client.
			Send( RestException<http::status::unauthorized>{move(e), move(req), "Could not get sessionInfo."}, move(stream) );
			co_return;
		}
		if( req.IsGet("/graphql") ){
			GraphQL( move(req), stream );
		}
		else
			HandleCustomRequest( move(req), move(stream) );
	}

#ifdef UNZIP
	α Unzip( string zip )ι->string{
		const u8 *compressed_data = in->mmap_mem;
		size_t compressed_size = in->mmap_size;
		void *uncompressed_data = NULL;
		size_t uncompressed_size;
		size_t max_uncompressed_size;
		size_t actual_in_nbytes;
		size_t actual_out_nbytes;
		enum libdeflate_result result;
		int ret = 0;

		if (compressed_size < GZIP_MIN_OVERHEAD ||
			compressed_data[0] != GZIP_ID1 ||
			compressed_data[1] != GZIP_ID2) {
			if (options->force && options->to_stdout)
				return full_write(out, compressed_data, compressed_size);
			msg("%"TS": not in gzip format", in->name);
			return -1;
		}

		/*
		* Use the ISIZE field as a hint for the decompressed data size.  It may
		* need to be increased later, however, because the file may contain
		* multiple gzip members and the particular ISIZE we happen to use may
		* not be the largest; or the real size may be >= 4 GiB, causing ISIZE
		* to overflow.  In any case, make sure to allocate at least one byte.
		*/
		uncompressed_size =
			get_unaligned_le32(&compressed_data[compressed_size - 4]);
		if (uncompressed_size == 0)
			uncompressed_size = 1;

		/*
		* DEFLATE cannot expand data more than 1032x, so there's no need to
		* ever allocate a buffer more than 1032 times larger than the
		* compressed data.  This is a fail-safe, albeit not a very good one, if
		* ISIZE becomes corrupted on a small file.  (The 1032x number comes
		* from each 2 bits generating a 258-byte match.  This is a hard upper
		* bound; the real upper bound is slightly smaller due to overhead.)
		*/
		if (compressed_size <= SIZE_MAX / 1032)
			max_uncompressed_size = compressed_size * 1032;
		else
			max_uncompressed_size = SIZE_MAX;

		do {
			if (uncompressed_data == NULL) {
				uncompressed_size = MIN(uncompressed_size,
					max_uncompressed_size);
				uncompressed_data = xmalloc(uncompressed_size);
				if (uncompressed_data == NULL) {
					msg("%"TS": file is probably too large to be "
						"processed by this program", in->name);
					ret = -1;
					goto out;
				}
			}

			result = libdeflate_gzip_decompress_ex(decompressor,
				compressed_data,
				compressed_size,
				uncompressed_data,
				uncompressed_size,
				&actual_in_nbytes,
				&actual_out_nbytes);

			if (result == LIBDEFLATE_INSUFFICIENT_SPACE) {
				if (uncompressed_size >= max_uncompressed_size) {
					msg("Bug in libdeflate_gzip_decompress_ex(): data expanded too much!");
					ret = -1;
					goto out;
				}
				if (uncompressed_size * 2 <= uncompressed_size) {
					msg("%"TS": file corrupt or too large to be "
						"processed by this program", in->name);
					ret = -1;
					goto out;
				}
				uncompressed_size *= 2;
				free(uncompressed_data);
				uncompressed_data = NULL;
				continue;
			}

			if (result != LIBDEFLATE_SUCCESS) {
				msg("%"TS": file corrupt or not in gzip format",
					in->name);
				ret = -1;
				goto out;
			}

			if (actual_in_nbytes == 0 ||
				actual_in_nbytes > compressed_size ||
				actual_out_nbytes > uncompressed_size) {
				msg("Bug in libdeflate_gzip_decompress_ex(): impossible actual_nbytes value!");
				ret = -1;
				goto out;
			}

			if (!options->test) {
				ret = full_write(out, uncompressed_data, actual_out_nbytes);
				if (ret != 0)
					goto out;
			}

			compressed_data += actual_in_nbytes;
			compressed_size -= actual_in_nbytes;

		} while (compressed_size != 0);
	out:
		free(uncompressed_data);
		return ret;
	}
#endif

	α Server::ReadSeverity( beast::error_code ec )ι->ELogLevel{
		auto level{ ELogLevel::Error };
		switch( ec.value() ){
		case net::error::operation_aborted: //EOF
			level = ELogLevel::Debug;
			break;
		case ssl::error::stream_truncated: //also known as an SSL "short read", peer closed the connection without performing the required closing handshake.
		case 0xA000416: //ERR_SSL_SSLV3_ALERT_CERTIFICATE_UNKNOWN: The client doesn't trust the certificate.
			level = ELogLevel::Trace;
			break;
		}
		return level;
	}

	α Server::SendOptions( const HttpRequest&& req )ι->http::message_generator{
		auto res = req.Response<http::empty_body>( http::status::no_content );
		res.set( http::field::access_control_allow_methods, "GET, POST, OPTIONS" );
		res.set( http::field::accept_encoding, "gzip" );
		res.set( http::field::access_control_allow_headers, "*" );//Access-Control-Allow-Origin, Authorization, Content-Type,
		res.set( http::field::access_control_expose_headers, "Authorization" );
		res.set( http::field::access_control_max_age, "7200" ); //2 hours chrome max
		return res;
	}
}