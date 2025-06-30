#pragma once
#include <boost/asio/experimental/parallel_group.hpp>
#include "Streams.h"
#include <jde/web/server/HttpRequest.h>
#include <jde/web/server/usings.h>
#include <jde/web/server/IApplicationServer.h>
#include <jde/web/server/IRequestHandler.h>

#define let const auto

namespace Jde::Web::Server{

namespace Internal{
	α Start( up<IRequestHandler>&& handler, up<Server::IApplicationServer>&& server )ε->void;
	α Stop( bool terminate=false )ι->void;
	α RunSocketSession( sp<IWebsocketSession>&& session )ι->void;
	α RemoveSocketSession( SocketId id )ι->void;
}
	α AppServerLocal()ι->bool;
	α AppGraphQLAwait( string&& q, UserPK userPK, SRCE )ι->up<TAwait<jvalue>>;
	Τ [[nodiscard]] α DoEof( T& stream )ι->net::awaitable<void, executor_type>{ beast::error_code ec; stream.socket().shutdown( tcp::socket::shutdown_send, ec ); co_return; }
	Τ [[nodiscard]] α DoEof( beast::ssl_stream<T>& stream )ι->net::awaitable<void, executor_type>{ co_await stream.async_shutdown(); }
	α HandleRequest( HttpRequest req, sp<RestStream> stream )ι->Sessions::UpsertAwait::Task;
	α ReadSeverity( beast::error_code ec )ι->ELogLevel;
	α GetRequestHandler()ι->IRequestHandler&;
	Τ [[nodiscard]] α RunSession( T& stream, beast::flat_buffer& buffer, tcp::endpoint userEndpoint, bool isSsl, uint32 connectionIndex, sp<net::cancellation_signal> cancel )ι->net::awaitable<void, executor_type>;
	α SendOptions( const HttpRequest&& req )ι->http::message_generator;
	α SendServerSettings( HttpRequest req, sp<RestStream> stream )ι->Sessions::UpsertAwait::Task;
	α SessionInfoAwait( SessionPK sessionPK, SRCE )ι->up<TAwait<Web::FromServer::SessionInfo>>;
}

namespace Jde::Web{
	Ŧ Server::RunSession( T& stream, beast::flat_buffer& buffer, tcp::endpoint userEndpoint, bool isSsl, uint32 connectionIndex, sp<net::cancellation_signal> /*cancel*/ )ι->net::awaitable<void, executor_type>{
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
				Internal::RunSocketSession( GetRequestHandler().GetWebsocketSession( ms<RestStream>(mu<T>(move(stream))), move(buffer), parser->release(), userEndpoint, connectionIndex) );
				co_return;
			}
			HttpRequest req{ parser->release(), move(userEndpoint), isSsl, connectionIndex };
			optional<http::message_generator> res;
			if( req.Method() == http::verb::options )
				res = SendOptions( move(req) );
			else if( req.IsPost("/ping") ){
				auto pingRes{ req.Response<http::empty_body>() };
				pingRes.set( http::field::summary, Jde::format("SSL={}", isSsl) );
				pingRes.prepare_payload();
				res = move(pingRes);
			}
			else if( req.IsGet("/serverSettings") ){
				SendServerSettings( move(req), ms<RestStream>(mu<T>(move(stream))) );
				co_return;
			}
			if( !res ){
				HandleRequest( move(req), ms<RestStream>(mu<T>(move(stream))) );
				co_return;//TODO handle keepalive
			}
			if( res && !res->keep_alive() ){
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
#undef let