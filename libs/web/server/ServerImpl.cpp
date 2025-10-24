#include "ServerImpl.h"
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/app/IApp.h>
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/web/server/IRequestHandler.h>
#include <jde/web/server/IWebsocketSession.h>
#include <jde/web/server/RestException.h>
#include <jde/ql/ql.h>
#include <jde/ql/QLAwait.h>
#include <jde/fwk/process/execution.h>
#define let const auto

namespace Jde::Web{
namespace Server{
	Ω detectSession( StreamType stream, tcp::endpoint userEndpoint, sp<net::cancellation_signal> cancel, Server::IRequestHandler* handler )ι->net::awaitable<void, executor_type>{
		beast::flat_buffer buffer;
		stream.expires_after( std::chrono::seconds(30) );// Set the timeout.
		auto [ec, isSsl] = co_await beast::async_detect_ssl( stream, buffer );// on_run
		if( ec ){
			CodeException{ ec, ELogTags::Server | ELogTags::Http, ELogLevel::Debug };
			co_return;
		}
		let index = handler->NextRequestId();
		if( isSsl ){
			beast::ssl_stream<StreamType> ssl_stream{ move(stream), handler->Context() };
			auto [ec, bytes_used] = co_await ssl_stream.async_handshake( ssl::stream_base::server, buffer.data() );
			if(ec){
				CodeException{ ec, ELogTags::Server | ELogTags::Http, ELogLevel::Warning };
				co_return;
			}

			buffer.consume(bytes_used);
			co_await RunSession( ssl_stream, buffer, move(userEndpoint), true, index, cancel, handler );
		}
		else
			co_await RunSession( stream, buffer, move(userEndpoint), false, index, cancel, handler );
	}

	Ω send( HttpRequest&& req, sp<RestStream> stream, jvalue j, sv contentType={}, SRCE )ι->void{
		auto res = req.Response( move(j), sl );
		if( contentType.size() )
			res.set( http::field::content_type, contentType );
		stream->AsyncWrite( move(res) );
	}

	Ω send( IRestException&& e, sp<RestStream> stream, sv contentType={} )ι->void{
		auto res = e.Response();
		if( contentType.size() )
			res.set( http::field::content_type, contentType );
		stream->AsyncWrite( move(res) );
	}

	Ω graphQL( HttpRequest req, sp<RestStream> stream, const vector<sp<DB::AppSchema>>& schemas )->QL::QLAwait<>::Task{
		constexpr sv contentType = "application/graphql-response+json";
		try{
			let returnRaw = req.Params().contains("raw");
			auto& query = req["query"]; THROW_IFX( query.empty(), RestException<http::status::bad_request>(SRCE_CUR, move(req), "No query sent.") );
			auto variables = Json::AsObject( parse(req["variables"]) );
			req.LogRead( query );
			auto result = co_await QL::QLAwait{ move(query), move(variables), req.UserPK(), schemas, returnRaw };
			jobject y{ {"data", result} };
			send( move(req), move(stream), move(y), contentType );
		}
		catch( IRestException& e ){
			send( move(e), move(stream), contentType );
			co_return;
		}
		catch( IException& e ){
			if( !empty(e.Tags() & ELogTags::Parsing) )
				send( RestException<http::status::bad_request>{move(e), move(req), "Query parsing failed."}, move(stream), contentType );
			else
				send( RestException{move(e), move(req), "Query failed."}, move(stream), contentType );
			co_return;
		}
		catch( exception& e ){
			send( RestException{ SRCE_CUR, move(req), "Query failed: {}", e.what() }, move(stream), contentType );
			co_return;
		}
	}

	Ω handleCustomRequest( HttpRequest req, sp<RestStream> stream, IRequestHandler* reqHandler )ι->IHttpRequestAwait::Task{
		try{
			HttpTaskResult result = co_await *( reqHandler->HandleRequest(move(req)) );
			THROW_IF( !result.Request, "Request not set." );
			send( move(*result.Request), move(stream), move(result.Json), {}, result.Source.value_or(SRCE_CUR) );
		}
		catch( IRestException& e ){
			send( move(e), move(stream) );
		}
		catch( IException& e ){
			e.SetLevel( ELogLevel::Critical );//no request object...
			send( RestException<>{move(e), move(req), "Error handling request."}, move(stream) );
		}
	}

	Ω initListener( typename tcp::acceptor::rebind_executor<executor_with_default>::other& acceptor, const tcp::endpoint& endpoint )ι->bool{
		beast::error_code ec;
		acceptor.open( endpoint.protocol(), ec );
		if( ec ){
			DBGT( ELogTags::App, "!initListener {}:{}", endpoint.address().to_string(), endpoint.port() );
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
			DBGT( ELogTags::App, "!initListener {}:{}", endpoint.address().to_string(), endpoint.port() );
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

	Ω loadServerCertificate( ssl::context& ctx, const Crypto::CryptoSettings& settings )ε->void{
		if( !fs::exists(settings.PrivateKeyPath) ){
			settings.CreateDirectories();
			Crypto::CreateKeyCertificate( settings );
		}
		ctx.set_options( ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::single_dh_use );
		let cert = IO::Load( settings.CertPath );
		ctx.use_certificate_chain( net::buffer(cert.data(), cert.size()) );

		ctx.set_password_callback( [=](uint, ssl::context_base::password_purpose){ return settings.Passcode; } );
		let key = IO::Load( settings.PrivateKeyPath );
		ctx.use_private_key( net::buffer(key.data(), key.size()), ssl::context::file_format::pem );
		static const string dhStatic =
			"-----BEGIN DH PARAMETERS-----\n"
			"MIIBCAKCAQEArzQc5mpm0Fs8yahDeySj31JZlwEphUdZ9StM2D8+Fo7TMduGtSi+\n"
			"/HRWVwHcTFAgrxVdm+dl474mOUqqaz4MpzIb6+6OVfWHbQJmXPepZKyu4LgUPvY/\n"
			"4q3/iDMjIS0fLOu/bLuObwU5ccZmDgfhmz1GanRlTQOiYRty3FiOATWZBRh6uv4u\n"
			"tff4A9Bm3V9tLx9S6djq31w31Gl7OQhryodW28kc16t9TvO1BzcV3HjRPwpe701X\n"
			"oEEZdnZWANkkpR/m/pfgdmGPU66S2sXMHgsliViQWpDCYeehrvFRHEdR9NV+XJfC\n"
			"QMUk26jPTIVTLfXmmwU0u8vUkpR7LQKkwwIBAg==\n"
			"-----END DH PARAMETERS-----\n";
		string dh = fs::exists( settings.DhPath ) ? IO::Load( settings.DhPath ) : dhStatic;
		ctx.use_tmp_dh( net::buffer(dh.data(), dh.size()) );
	}

	Ω listen( tcp::endpoint endpoint, sp<IRequestHandler> handler )ι->net::awaitable<void, executor_type>{
		typename tcp::acceptor::rebind_executor<executor_with_default>::other acceptor{ co_await net::this_coro::executor };
		if( !initListener(acceptor, endpoint) ){
			DBGT( ELogTags::App, "!initListener" );
			co_return;
		}

		TRACET( ELogTags::App, "Web Server accepting." );
		handler->Start();
		while( (co_await net::this_coro::cancellation_state).cancelled() == net::cancellation_type::none ){
			auto [ec, sock] = co_await acceptor.async_accept();
			let exec = sock.get_executor();
			let userEndpoint = sock.remote_endpoint();
			if( !ec ){
				auto cancelSignal = ms<net::cancellation_signal>();
				net::co_spawn(
					exec,
					detectSession( StreamType(move(sock)), move(userEndpoint), cancelSignal, handler.get() ),
					net::bind_cancellation_slot(cancelSignal->slot(),
					net::detached)
				);// We dont't need a strand, since the awaitable is an implicit strand.
				Execution::AddCancelSignal( cancelSignal );
			}
		}
	}

	α Internal::Start( sp<IRequestHandler> handler )ε->void{
		loadServerCertificate( handler->Context(), handler->Settings().Crypto() );

		let port = handler->Settings().Port();
		let addressString = handler->Settings().Address();
		let address = tcp::endpoint{ net::ip::make_address(addressString), port };
		net::co_spawn(
			*Executor(),
			listen( address, handler ),
			net::bind_cancellation_slot(handler->CancelSignal()->slot(), net::detached)
		);
		Execution::AddCancelSignal( handler->CancelSignal() );
		Execution::Run();
		handler->BlockTillStarted(); // wait for boost to end.
		INFOT( ELogTags::App, "Web Server started:  {}:{}.", address.address().to_string(), address.port() );
	}

	α Internal::Stop( sp<IRequestHandler>&& handler, bool /*terminate*/ )ι->void{
		handler->Stop();
	}

	concurrent_flat_map<SessionPK, sp<IWebsocketSession>> _socketSessions;
	α Internal::RunSocketSession( sp<IWebsocketSession>&& session )ι->void{
		let id = session->Id();
		_socketSessions.emplace( id, session );
		session->Run();
	}

	α Internal::RemoveSocketSession( SocketId id )ι->void{
		TRACET( ELogTags::SocketServerRead, "erased socket: {:x}", _socketSessions.erase( id ) );
	}
}
	α Server::HandleRequest( HttpRequest req, sp<RestStream> stream, IRequestHandler* reqHandler )ι->TAwait<sp<SessionInfo>>::Task{
		try{
			req.SessionInfo = co_await Sessions::UpsertAwait( req.Header("authorization"), req.UserEndpoint.address().to_string(), false, reqHandler->AppServer() );
		}
		catch( IException& e ){
			send( RestException<http::status::unauthorized>{move(e), move(req), "Could not get sessionInfo."}, move(stream) );
			co_return;
		}
		if( req.IsGet("/graphql") && !reqHandler->PassQL() )
			graphQL( move(req), stream, reqHandler->Schemas() );
		else
			handleCustomRequest( move(req), move(stream), reqHandler );
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
	α Server::SendServerSettings( HttpRequest req, sp<RestStream> stream, sp<App::IApp> appClient )ι->Sessions::UpsertAwait::Task{
		jobject j;
		j["restSessionTimeout"] = Chrono::ToString( Sessions::RestSessionTimeout() );
		j["serverInstance"] = appClient->InstancePK();
		try{
			let session = co_await Sessions::UpsertAwait( req.Header("authorization"), req.UserEndpoint.address().to_string(), false, appClient, false );
			j["active"] = (bool)session;
		}
		catch( IException& e ){
			j["active"] = false;
			e.SetLevel( ELogLevel::Trace );
		}

		send( move(req), move(stream), j, "application/json" );
	}
}