#pragma once
#include <boost/asio/experimental/parallel_group.hpp>
#include "Sessions.h"
#include "RestException.h"
#include <jde/web/server/Streams.h>
#include <jde/web/server/IHttpRequestAwait.h>

//#define var const auto
#define Φ ΓWS auto
struct IHttpRequestAwait;
namespace Jde::Web::Server{
	α MaxLogLength()ι->uint16;

	struct IApplicationServer;//TODO! move this here or IRequestHandler there
	struct IRequestHandler{
		virtual ~IRequestHandler()=default; //msvc error
		β HandleRequest( HttpRequest&& req, SRCE )ι->up<IHttpRequestAwait> =0; //abstract, can't return a copy.
		β RunWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->void =0;
	};
	α GetRequestHandler()ι->IRequestHandler&;
	α AppGraphQLAwait( string&& q, UserPK userPK, SRCE )ι->up<TAwait<json>>;
	α SessionInfoAwait( SessionPK sessionPK, SRCE )ι->up<TAwait<App::Proto::FromServer::SessionInfo>>;
	struct CancellationSignals;
//	α HasStarted()ι->bool;
	Φ Start( up<IRequestHandler> handler, up<Server::IApplicationServer>&& server )ε->void;
	Φ Stop( bool terminate=false )ι->void;

	α ReadSeverity( beast::error_code ec )ι->ELogLevel;
	α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void;
	α Send( HttpRequest&& req, sp<RestStream> stream, json j, SRCE )ι->void;
	α Send( IRestException&& e, sp<RestStream> stream )ι->void;

	Ŧ DoEof( T& stream )ι->net::awaitable<void, executor_type>{
		beast::error_code ec;
		stream.socket().shutdown( tcp::socket::shutdown_send, ec );
		co_return;
	}

	Τ [[nodiscard]] α DoEof( beast::ssl_stream<T>& stream )ι->net::awaitable<void, executor_type>{
		co_await stream.async_shutdown();
	}

	Τ [[nodiscard]] α RunSession( T& stream, beast::flat_buffer& buffer, tcp::endpoint userEndpoint, bool isSsl, uint32 connectionIndex, sp<net::cancellation_signal> cancel )ι->net::awaitable<void, executor_type>;

	[[nodiscard]] α DetectSession( StreamType stream, tcp::endpoint userEndpoint, sp<net::cancellation_signal> cancel )ι->net::awaitable<void, executor_type>;
	α InitListener( typename tcp::acceptor::rebind_executor<executor_with_default>::other& acceptor, const tcp::endpoint& endpoint )ι->bool;
// Accepts incoming connections and launches the sessions.
	[[nodiscard]] α Listen( tcp::endpoint endpoint )ι->net::awaitable<void, executor_type>;
	namespace Internal{
		α SendOptions( const HttpRequest&& req )ι->http::message_generator;
		//Ŧ HandleRequest( HttpRequest req, up<T> stream, SessionPK sessionId )->Task;
		α HandleRequest( HttpRequest req, sp<RestStream> stream )ι->Sessions::UpsertAwait::Task;
		α HandleCustomRequest( HttpRequest req, sp<RestStream> stream )ι->IHttpRequestAwait::Task;
	}
}

namespace Jde::Web{
	Ŧ Server::RunSession( T& stream, beast::flat_buffer& buffer, tcp::endpoint userEndpoint, bool isSsl, uint32 connectionIndex, sp<net::cancellation_signal> cancel )ι->net::awaitable<void, executor_type>{
    optional<http::request_parser<http::string_body>> parser;// a new parser must be used for every message so we use an optional to reconstruct it every time.
    parser.emplace();
    parser->body_limit(10000); // Apply a reasonable limit to the allowed size  of the body in bytes to prevent abuse.
    auto [ec, bytes_transferred] = co_await http::async_read( stream, buffer, *parser );
    if( ec == http::error::end_of_stream )
      co_await DoEof( stream );
    if( ec ){
      CodeException{ ec, ELogTags::HttpServerRead, ReadSeverity(ec) };
			co_return;
		}

    // this can be while ((co_await net::this_coro::cancellation_state).cancelled() == net::cancellation_type::none) on most compilers
    for( auto cs = co_await net::this_coro::cancellation_state; cs.cancelled() == net::cancellation_type::none; cs = co_await net::this_coro::cancellation_state ){
      if( websocket::is_upgrade(parser->get()) ){
        beast::get_lowest_layer(stream).expires_never();// Disable the timeout. The websocket::stream uses its own timeout settings.
				GetRequestHandler().RunWebsocketSession( ms<RestStream>(mu<T>(move(stream))), move(buffer), parser->release(), userEndpoint, connectionIndex );
				co_return;
      }
			HttpRequest req{ parser->release(), move(userEndpoint), isSsl, connectionIndex };
			optional<http::message_generator> res;
			if( req.Method() == http::verb::options )
				res = Internal::SendOptions( move(req) );
			else if( req.IsPost("/ping") ){
				auto pingRes{ req.Response<http::empty_body>() };
				pingRes.set( http::field::summary, Jde::format("SSL={}", isSsl) );
				pingRes.prepare_payload();
				res = move(pingRes);
			}
			if( !res ){
				Internal::HandleRequest( move(req), ms<RestStream>(mu<T>(move(stream))) );
				co_return;//TODO handle keepalive
			}
			if( res && !res->keep_alive() ){
				//http::message_generator msg{ move(res) };
				auto [ec, sz] = co_await beast::async_write( stream, move(*res) );
				if( ec )
					CodeException{ ec, ELogTags::HttpServerWrite, ELogLevel::Debug };
				co_return;
			}
      parser.reset();// we must use a new parser for every async_read
      parser.emplace();
      parser->body_limit( 10000 );
	    http::message_generator msg{ move(*res) };
      auto [_, ec_r, sz_r, ec_w, sz_w ] = co_await net::experimental::make_parallel_group(
        http::async_read( stream, buffer, *parser, net::deferred ),
        beast::async_write( stream, move(msg), net::deferred ) ).async_wait( net::experimental::wait_for_all(), net::as_tuple(net::use_awaitable_t<executor_type>{}) );
      if (ec_r){
        CodeException{ ec, ELogTags::HttpServerRead, ELogLevel::Trace };
				co_return;
			}
      if (ec_w){
        CodeException{ ec, ELogTags::HttpServerWrite, ELogLevel::Trace };
				co_return;
			}
	  }
	}
}
#undef Φ