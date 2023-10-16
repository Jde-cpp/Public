#include "RestServer.h"
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "../../../Framework/source/math/MathUtilities.h"
#define var const auto

namespace Jde::Web::Rest
{
	const LogTag& _logLevel = Logging::TagLevel( "rest" );
#define CHECK_EC(ec, ...) if( ec ){ CodeException x(ec __VA_OPT__(,) __VA_ARGS__); return; }

	IListener::IListener( PortType port )ε:
		_pIOContext{ IO::AsioContextThread::Instance() },
		_acceptor{ _pIOContext->Context(), tcp::endpoint{ Settings::Get("net/ip").value_or("v6")=="v6" ? tcp::v6() : tcp::v4(), port } }
  {
    beast::error_code ec;
    // Acceptor.open( endpoint.protocol(), ec ); THROW_IF( ec, "open" );
    // Acceptor.set_option(net::socket_base::reuse_address(true), ec); THROW_IF( ec, "reuse_address" );
    // Acceptor.bind(endpoint, ec); THROW_IF( ec, "bind" );
		INFO( "Listening to Rest calls on port '{}'", port );
    _acceptor.listen( net::socket_base::max_listen_connections, ec ); THROW_IF( ec, "listen" );
  }

  α IListener::DoAccept()ι->void
	{
		sp<IListener> sp_ = static_pointer_cast<IListener>( MakeShared() );
		_acceptor.async_accept( net::make_strand(_pIOContext->Context()), beast::bind_front_handler(&IListener::OnAccept, sp_) );
	}

  α IListener::OnAccept( beast::error_code ec, tcp::socket socket )ι->void
  {
		LOG( "ISession::OnAccept()" );
		CHECK_EC( ec );
		CreateSession( std::move(socket) )->Run();
    DoAccept();
  }

	α ISession::DoRead()ι->void
  {
    _stream.expires_after( std::chrono::seconds(30) );
    http::async_read( _stream, _buffer, _request, beast::bind_front_handler( &ISession::OnRead, MakeShared()) );
  }

  α ISession::OnRead( beast::error_code ec, std::size_t bytes_transferred )ι->void
  {
    boost::ignore_unused(bytes_transferred);
    if(ec == http::error::end_of_stream)
      return DoClose();

		CHECK_EC( ec );

		HandleRequest( Request{ MakeShared() } );
  }

	α ISession::OnWrite( bool close, beast::error_code ec, std::size_t bytes_transferred )ι->void
  {
      boost::ignore_unused(bytes_transferred);
			CHECK_EC( ec );
      DoClose();
  }

	α ISession::DoClose()ι->void
  {
      beast::error_code ec;
      _stream.socket().shutdown(tcp::socket::shutdown_send, ec);
  }
	Ŧ SetResponse( http::response<T>& res, bool keepAlive )ι->void
	{
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::access_control_allow_origin, "*");
		res.keep_alive( keepAlive );
    res.prepare_payload();
	}

	Ŧ SetBodyResponse( http::response<T>& res, bool keepAlive )ι->void
	{
		res.set(http::field::content_type, "application/json");
		SetResponse<T>( res, keepAlive );
		//https://www.boost.org/doc/libs/1_76_0/boost/beast/http/field.hpp
	}
	α ISession::Send( string&& value, Request&& req )ι->void
	{
		auto res = ms<http::response<http::string_body>>( std::piecewise_construct, std::make_tuple(std::move(value)), std::make_tuple(http::status::ok, req.ClientRequest().version()) );
		SetBodyResponse( *res, req.ClientRequest().keep_alive() );
		Send( move(res), move(req.Session) );
	}
	α ISession::SendOptions( Request&& req )ι->void
	{
		auto y = ms<http::response<http::empty_body>>( http::status::no_content, req.ClientRequest().version() );
    y->set(http::field::access_control_allow_methods, "GET, POST");
    y->set(http::field::access_control_allow_headers, "*");
    y->set(http::field::access_control_max_age, "86400");
		SetResponse( *y, req.ClientRequest().keep_alive() );
    Send( move(y), move(req.Session) );
	}

	α ISession::Send( Exception&& e, Request&& req )ι->void
	{
		var p = dynamic_cast<IRequestException*>( &e );
		string what = p ? p->what() : format("Internal Server Error:  '{:x}'.", e.Code);
		Send( p ? p->Status() : http::status::internal_server_error, what, move(req) );
	}
	α ISession::Send( const IRequestException&& e, Request&& req )ι->void
	{
		Send( e.Status(), e.what(), move(req) );
	}

	α ISession::Send( http::status status, string what, Request&& req )ι->void
	{
    auto y = ms<http::response<http::string_body>>( status, req.ClientRequest().version() );
		var message = format( "{{\"message\": \"{}\"}}", move(what) );
		Dbg( message );
		SetBodyResponse( *y, req.ClientRequest().keep_alive() );
    y->body() = message;
    y->prepare_payload();
		Send( move(y), move(req.Session) );
	}

	static flat_map<SessionPK, sp<SessionInfo>> _sessions; shared_mutex _sessionMutex;
	α AddSession2( sp<SessionInfo>& p )ι->void
	{
		ul _{ _sessionMutex };
		_sessions.emplace( p->session_id(), p );
	}
	α ISession::AddSession( UserPK userId )ι->sp<SessionInfo>
	{
		auto p = ms<SessionInfo>();
		auto pTimestamp = mu<google::protobuf::Timestamp>();
		pTimestamp->set_seconds( time(nullptr)+60*60 );
		p->set_allocated_expiration( pTimestamp.release() );
		p->set_session_id( Math::Random() );
		p->set_user_id( userId );
		AddSession2( p );
		return p;
	}

	α FetchSessionInfo( SessionPK sessionId, HCoroutine h )ι->Task
	{
		try
		{
			sp<SessionInfo> p{ (co_await Logging::Server::FetchSessionInfo(sessionId)).UP<SessionInfo>().release() };
			h.promise().get_return_object().SetResult( p );
			AddSession2(p);
		}
		catch( Exception& e )
		{
			h.promise().get_return_object().SetResult( move(e) );
		}
		h.resume();
	}

	α SessionInfoAwait::await_ready()ι->bool
	{
		sl _{ _sessionMutex };
		if( auto p = _sessions.find( _sessionId ); p!=_sessions.end() )
			_result.Set( p->second );
		else if( Logging::Server::IsLocal() )//if local, should have been in _sessions.
			_result.Set( sp<SessionInfo>{} );
		return _result.HasShared();
	}
	α SessionInfoAwait::await_suspend( HCoroutine h )ι->void{ IAwaitCache::await_suspend(h); FetchSessionInfo( _sessionId, h ); }

	α ParseUri( string&& uri )->tuple<string,flat_map<string,string>>
	{
	  var target{ uri.substr(0, uri.find('?')) };
		flat_map<string,string> params;
		if( target.size()+1<uri.size() )
		{
			var start = target.size()+1;
			sv paramString = sv{ uri.data()+start, uri.size()-start };
			var paramStringSplit = Str::Split( paramString, '&' );
			for( auto param : paramStringSplit )
			{
				var keyValue = Str::Split( param, '=' );
				params[string{keyValue[0]}]=keyValue.size()==2 ? string{keyValue[1]} : string{};
			}
		}
		return make_tuple( target, params );
	}
	α ISession::HandleRequest( Request req )ι->Task
	{
		auto [target, params] = ParseUri( Ssl::DecodeUri(string{req.ClientRequest().target()}) );
		LOG( "{}={}", target, req.ClientRequest().body() );
		var sessionString{ req.ClientRequest().base()["session-id"] };
		try
		{
			if( !sessionString.empty() )
			{
				var sessionId = Str::TryToUInt( string{sessionString}, nullptr, 16 ); THROW_IFX(!sessionId, RequestException<http::status::bad_request>(SRCE_CUR, "Could not create sessionId {}.", sessionString); )
				req.SessionInfoPtr = ( co_await FetchSessionInfo(*sessionId) ).UP<SessionInfo>();
				if( req.SessionInfoPtr && req.SessionInfoPtr->expiration().seconds()<time(nullptr) )
					RequestException<http::status::unauthorized>( SRCE_CUR, "session timeout '{}'.", sessionString);
			}
	    if( req.Method() == http::verb::get )
			{
				if( target=="/graphql" )
				{
					auto& query = params["query"]; THROW_IFX( query.empty(), RequestException<http::status::bad_request>(SRCE_CUR, "No query sent.") );
					string threadDesc = format( "[{:x}]{}", req.SessionInfoPtr ? req.SessionInfoPtr->session_id() : 0, target );
					var y = ( co_await DB::CoQuery(move(query), req.UserId(), threadDesc) ).UP<nlohmann::json>();
					Send( move(*y), move(req) );
				}
			}
			else if( req.Method() == boost::beast::http::verb::options )
				SendOptions( move(req) );

			if( req.Session )
				HandleRequest( move(target), move(params), move(req) );
		}
		catch( Exception& e )
		{
			Send( move(e), move(req) );
		}
	}

	α ISession::Send( const json& payload, Request&& req )ι->void
	{
		Send( payload.dump(), move(req) );
	}
}