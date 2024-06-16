#pragma once
#include <boost/beast/http.hpp>//
#include <boost/beast/websocket.hpp>//
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/ssl.hpp>
#include "WebSocketSession.h"
//#include "HttpRequest.h"
#include "Sessions.h"
#include "../rest/RestException.h"
#define var const auto

namespace Jde::Web::Flex{
	struct CancellationSignals;
	using StreamType = typename beast::tcp_stream::rebind_executor<executor_with_default>::other;
	using Streams = std::variant<StreamType, beast::ssl_stream<StreamType>>;	
	α HasStarted()ι->bool;
	α Start( PortType port, sv address={}, uint8 threadCount=1 )ε->void;

	α Fail( beast::error_code ec, char const* what )->void{
		if( ec == net::ssl::error::stream_truncated )// ssl::error::stream_truncated, also known as an SSL "short read", peer closed the connection without performing the required closing handshake.
			return;

		std::cerr << what << ": " << ec.message() << "\n";
	}
	α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), ec.value()==(int)boost::beast::error::timeout ? ELogLevel::Debug : ELogLevel::Error };
  }

	α Send( HttpRequest&& req, Streams&& stream, Rest::IRestException&& e )->void{//->net::awaitable<void, executor_type>{
		auto res = e.Response( move(req) );
//		beast::error_code ec; uint sz;
//		if( stream.index()==0 )
//			std::tie( ec, sz ) = co_await beast::async_write( std::get<0>(stream), move(res) );
//		else
		beast::ssl_stream<StreamType>& a = std::get<1>(stream);
		http::message_generator msg{ move(res) };
		beast::async_write( a, move(msg), beast::bind_front_handler(OnWrite) );
//			std::tie( ec, sz ) = co_await beast::async_write( a, move(msg) );
//		if( ec )
//			Fail( ec, "write" );
		//co_return;
	}

	Ŧ DoEof( T& stream )->net::awaitable<void, executor_type>{
		beast::error_code ec;
		stream.socket().shutdown( tcp::socket::shutdown_send, ec );
		co_return;
	}

	Τ [[nodiscard]] α DoEof( beast::ssl_stream<T>& stream )->net::awaitable<void, executor_type>{ 
		co_await stream.async_shutdown(); 
	}

	Τ [[nodiscard]] α RunSession( T& stream, beast::flat_buffer& buffer, bool isSsl )->net::awaitable<void, executor_type>;

	[[nodiscard]] α DetectSession( StreamType stream, net::ssl::context& ctx, tcp::endpoint userEndpoint)->net::awaitable<void, executor_type>;
	α InitListener( typename tcp::acceptor::rebind_executor<executor_with_default>::other& acceptor, const tcp::endpoint& endpoint )->bool;
// Accepts incoming connections and launches the sessions.
	[[nodiscard]] α Listen( ssl::context& ctx, tcp::endpoint endpoint, CancellationSignals& sig )->net::awaitable<void, executor_type>;
	namespace Internal{
		α SendOptions( const HttpRequest&& req )->http::message_generator;
		//Ŧ HandleRequest( HttpRequest req, up<T> stream, SessionPK sessionId )->Task;
		α HandleRequest( HttpRequest req, Streams )->Task;
	}
}

namespace Jde::Web{
	Ŧ Flex::RunSession( T& stream, beast::flat_buffer& buffer, tcp::endpoint userEndpoint, bool isSsl )->net::awaitable<void, executor_type>{
    optional<http::request_parser<http::string_body>> parser;// a new parser must be used for every message so we use an optional to reconstruct it every time.
    parser.emplace();
    parser->body_limit(10000); // Apply a reasonable limit to the allowed size  of the body in bytes to prevent abuse.
    auto [ec, bytes_transferred] = co_await http::async_read( stream, buffer, *parser );
    if( ec == http::error::end_of_stream )
        co_await DoEof( stream );
    if( ec )
      co_return Fail( ec, "read" );

    // this can be while ((co_await net::this_coro::cancellation_state).cancelled() == net::cancellation_type::none) on most compilers
    for( auto cs = co_await net::this_coro::cancellation_state; cs.cancelled() == net::cancellation_type::none; cs = co_await net::this_coro::cancellation_state ){
      if( websocket::is_upgrade(parser->get()) ){
        beast::get_lowest_layer(stream).expires_never();// Disable the timeout. The websocket::stream uses its own timeout settings.
        co_await RunWebsocketSession( stream, buffer, parser->release() );
        co_return;
      }
			HttpRequest req{ parser->release(), move(userEndpoint) };
			optional<http::message_generator> res;
			if( req.Method() == http::verb::options )
				res = Internal::SendOptions( move(req) );
			if( req.Method() == http::verb::post && req.StringBody()=="PING" ){
				auto pingRes{ req.Response<http::string_body>() };
				pingRes.body() = Jde::format( "SSL={}", isSsl );
				pingRes.prepare_payload();
				res = move(pingRes);
			}
			if( !res ){
				Internal::HandleRequest( move(req), Streams{move(stream)} );
				co_return;
			}
			if( res && !res->keep_alive() ){
				//http::message_generator msg{ move(res) };
				auto [ec, sz] = co_await beast::async_write( stream, move(*res) );
				if( ec )
					Fail(ec, "write");
				co_return;
			}
      parser.reset();// we must use a new parser for every async_read
      parser.emplace();
      parser->body_limit( 10000 );
	    http::message_generator msg{ move(*res) };
      auto [_, ec_r, sz_r, ec_w, sz_w ] = co_await net::experimental::make_parallel_group(
        http::async_read( stream, buffer, *parser, net::deferred ),
        beast::async_write( stream, move(msg), net::deferred ) ).async_wait( net::experimental::wait_for_all(), net::as_tuple(net::use_awaitable_t<executor_type>{}) );
      if (ec_r)
        co_return Fail( ec_r, "read" );
      if (ec_w)
        co_return Fail( ec_w, "write" );
	  }
	}
	namespace Flex{
		α Internal::SendOptions( const HttpRequest&& req )->http::message_generator{
			auto res = req.Response<http::empty_body>( http::status::no_content );
			res.set( http::field::access_control_allow_methods, "GET, POST, OPTIONS" );
			res.set( http::field::access_control_allow_headers, "*" );
			res.set( http::field::access_control_max_age, "7200" ); //2 hours chrome max
			return res;
		}
		//Ŧ
		α Internal::HandleRequest( HttpRequest req, Streams stream )->Task{
			var authorization{ req["authorization"] };
			if( !authorization.empty() ){
				try{
					optional<SessionPK> sessionId;
					if( sessionId = Str::TryTo<SessionPK>(string{authorization}, nullptr, 16);  !sessionId )
						throw Rest::RestException<http::status::unauthorized>{ "Could not create sessionId:  '{}'.", authorization };
					if( auto pInfo = Sessions::UpdateExpiration(*sessionId, req.UserEndpoint); !pInfo ){
						THROW_IF( Logging::Server::IsLocal(), "[{}]Session not found.", *sessionId );
						auto pServerInfo = awaitp( SessionInfo, Logging::Server::FetchSessionInfo(*sessionId, req.UserEndpoint) ); THROW_IF( !pServerInfo, "[{}]AppServer did not have sessionInfo.", *sessionId );
						req.SessionInfo = { *pServerInfo, req.UserEndpoint };
						Sessions::Upsert( req.SessionInfo );
					}
					// else if( !Logging::Server::IsLocal() && pInfo->LastServerUpdate<steady_clock::now()-5min ) //TODO update expiration on server also.
					// 	await( SessionInfo, Logging::Server::UpdateSession(sessionId, req.UserEndpoint) ); THROW_IF( !pServerInfo, "[{}]AppServer did not have sessionInfo.", sessionId );
				}
				catch( IException& e ){
					Send( move(req), move(stream), Rest::RestException<http::status::unauthorized>(SRCE_CUR, move(e), "Could not get sessionInfo.") );
					co_return;
				}
			}
			else{
				Sessions::Info newSession{ Sessions::GetNewSessionId(), req.UserEndpoint };//TODO create sessionId.
				Sessions::Upsert( newSession );
				req.SessionInfo = newSession;
			}
	    if( req.Method() == http::verb::get && req.Target()=="/graphql" ){
				try{
					auto& query = req["query"s]; THROW_IFX( query.empty(), Rest::RestException<http::status::bad_request>(SRCE_CUR, "No query sent.") );
					var sessionId = req.SessionInfo.SessionId;
					TRACE( "[{:x}] - {}", sessionId, query );
					string threadDesc = Jde::format( "[{:x}]{}", sessionId, req.Target() );
					var y = await( json, DB::CoQuery(move(query), req.UserId(), threadDesc) );
					TRACET( _logTagResponse, "[{:x}] - {}", sessionId, y.dump() );
					Send( move(y), move(stream) );
				}
				catch( IException& e ){
					Send( move(req), move(stream), Rest::RestException<http::status::unauthorized>(SRCE_CUR, move(e), "Could not get sessionInfo.") );
					co_return;
				}
			}

			//handle graphql.
			//handle custom.
			co_return;
		}

	}
}