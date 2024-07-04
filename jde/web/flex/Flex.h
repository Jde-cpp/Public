#pragma once
#include <boost/beast/http.hpp>//
#include <boost/beast/websocket.hpp>//
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/ssl.hpp>
//#include "WebSocketSession.h"
//#include "HttpRequest.h"
#include "Sessions.h"
#include "RestException2.h"
#include "HttpRequestAwait.h"
#define var const auto

namespace Jde::Web::Flex{
	struct IRequestHandler{
		β HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> =0;
		β RunWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint )ι->void =0;
	};
	α GetRequestHandler()ι->sp<IRequestHandler>;
	α GetIOContext()ι->sp<net::io_context>;

	struct CancellationSignals;
	α HasStarted()ι->bool;
	α Start( sp<IRequestHandler> )ε->void;
	α Stop( bool terminate=false )ι->void;

	α Fail( beast::error_code ec, char const* what )ι->void;
	α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void;
	α Send( HttpRequest&& req, sp<RestStream> stream, json j )ι->void;
	α Send( IRestException&& e, sp<RestStream> stream )ι->void;

	Ŧ DoEof( T& stream )ι->net::awaitable<void, executor_type>{
		beast::error_code ec;
		stream.socket().shutdown( tcp::socket::shutdown_send, ec );
		co_return;
	}

	Τ [[nodiscard]] α DoEof( beast::ssl_stream<T>& stream )ι->net::awaitable<void, executor_type>{
		co_await stream.async_shutdown();
	}

	Τ [[nodiscard]] α RunSession( T& stream, beast::flat_buffer& buffer, bool isSsl )ι->net::awaitable<void, executor_type>;

	[[nodiscard]] α DetectSession( StreamType stream, net::ssl::context& ctx, tcp::endpoint userEndpoint)ι->net::awaitable<void, executor_type>;
	α InitListener( typename tcp::acceptor::rebind_executor<executor_with_default>::other& acceptor, const tcp::endpoint& endpoint )ι->bool;
// Accepts incoming connections and launches the sessions.
	[[nodiscard]] α Listen( ssl::context& ctx, tcp::endpoint endpoint )ι->net::awaitable<void, executor_type>;
	namespace Internal{
		α SendOptions( const HttpRequest&& req )ι->http::message_generator;
		//Ŧ HandleRequest( HttpRequest req, up<T> stream, SessionPK sessionId )->Task;
		α HandleRequest( HttpRequest req, sp<RestStream> stream )ι->Sessions::UpsertAwait::Task;
		α HandleCustomRequest( HttpRequest req, sp<RestStream> stream )ι->HttpTask;
	}
}

namespace Jde::Web{
	Ŧ Flex::RunSession( T& stream, beast::flat_buffer& buffer, tcp::endpoint userEndpoint, bool isSsl )ι->net::awaitable<void, executor_type>{
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
				GetRequestHandler()->RunWebsocketSession( ms<RestStream>(mu<T>(move(stream))), move(buffer), parser->release(), userEndpoint );
				co_return;
      }
			HttpRequest req{ parser->release(), move(userEndpoint) };
			optional<http::message_generator> res;
			if( req.Method() == http::verb::options )
				res = Internal::SendOptions( move(req) );
			if( req.Method() == http::verb::post && req.StringBody()=="PING" ){
				auto pingRes{ req.Response<http::empty_body>() };
				pingRes.set( http::field::summary, Jde::format("SSL={}", isSsl) );
				pingRes.prepare_payload();
				res = move(pingRes);
			}
			if( req.Method() == http::verb::get && req.Target()=="/isSsl" ){//TODO: remove this, ssl client isn't returning headers...
				auto pingRes{ req.Response<http::string_body>() };
				pingRes.body() = Jde::format( "SSL={}", isSsl );
				pingRes.prepare_payload();
				res = move(pingRes);
			}
			if( !res ){
				Internal::HandleRequest( move(req), ms<RestStream>(mu<T>(move(stream))) );
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
}