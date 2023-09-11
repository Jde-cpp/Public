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
    Request = {};
    Stream.expires_after( std::chrono::seconds(30) );
    http::async_read( Stream, Buffer, Request, beast::bind_front_handler( &ISession::OnRead, MakeShared()) );
  }

  α ISession::OnRead( beast::error_code ec, std::size_t bytes_transferred )ι->void
  {
    boost::ignore_unused(bytes_transferred);
    if(ec == http::error::end_of_stream)
      return DoClose();

		CHECK_EC( ec );

		HandleRequest( std::move(Request), MakeShared() );
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
      Stream.socket().shutdown(tcp::socket::shutdown_send, ec);
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
	α ISession::Send( string value, http::request<http::string_body>&& req )ι->void
	{
		http::response<http::string_body> res{ std::piecewise_construct, std::make_tuple(std::move(value)), std::make_tuple(http::status::ok, req.version()) };
		SetBodyResponse( res, req.keep_alive() );
		Send(move(res));
	}
	α ISession::SendOptions( http::request<http::string_body>&& req )ι->void
	{
		http::response<http::empty_body> res{ http::status::no_content, req.version() };
    res.set(http::field::access_control_allow_methods, "GET, POST");
    res.set(http::field::access_control_allow_headers, "*");
    res.set(http::field::access_control_max_age, "86400");
		SetResponse( res, req.keep_alive() );
    Send(move(res));
	}

	α ISession::Error( Exception&& e, http::request<http::string_body>&& req )ι->void
	{
		var p = dynamic_cast<IRequestException*>( &e );
		Error( p ? p->Status() : http::status::internal_server_error, e.what(), move(req) );
	}
	α Error( const IRequestException& e, string what, http::request<http::string_body>&& req )ι->void
	{
		Error( e.Status(), what, move(req) );
	}

	α ISession::Error( http::status status, string what, http::request<http::string_body>&& req )ι->void
	{
    http::response<http::string_body> res{status, req.version()};
		var message = format( "{{\"message\": \"{}\"}}", move(what) );
		Dbg( message );
		SetBodyResponse( res, req.keep_alive() );
    res.body() = message;
    res.prepare_payload();
		Send( move(res) );
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

	α FetchSessionInfo( UserPK userId, HCoroutine h )ι->Task
	{
		try
		{
			sp<SessionInfo> p{ (co_await Logging::Server()->FetchSessionInfo(userId)).UP<SessionInfo>().release() };
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
		else if( Logging::Server()->IsLocal() )
			_result.Set( sp<SessionInfo>{} );
		return _result.HasShared();
	}
	α SessionInfoAwait::await_suspend( HCoroutine h )ι->void{ IAwait::await_suspend(h); FetchSessionInfo( _sessionId, h ); }

	α ParseUri( string&& uri )->tuple<string,flat_map<string,string>>
	{
	  var target{ uri.substr(0, uri.find('?')-1) };
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
	α ISession::HandleRequest( http::request<http::string_body>&& req, sp<ISession> s )ι->Task
	{
		auto [target, params] = ParseUri( Ssl::DecodeUri(string{req.target()}) );
		DBG( "{}={}", target, req.body() );
		var sessionString{ req.base()["session-id"] };
		up<SessionInfo> pSessionInfo;
		try
		{
			auto handled{ false };
			if( !sessionString.empty() )
			{
				var sessionId = Str::TryToUInt( string{sessionString}, nullptr, 16 ); THROW_IFX(!sessionId, RequestException<http::status::bad_request>( SRCE_CUR, "Could not create sessionId {}.", sessionString); )
				pSessionInfo = ( co_await FetchSessionInfo(*sessionId) ).UP<SessionInfo>();
				if( pSessionInfo && pSessionInfo->expiration().seconds()<time(nullptr) )
					RequestException<http::status::unauthorized>( SRCE_CUR, "session timeout '{}'.", sessionString);
				//check expired
			}
			var userId = pSessionInfo ? pSessionInfo->user_id() : 0;
	    if( req.method() == http::verb::get )
			{
				if( target.starts_with("/graphql?") && target.size()>9 )
				{
					sv target2 = "/graphql";
					uint start = target2.size()+1;
					auto psz = iv{target.data()+start, target.size()-start};
					var params = Str::Split<iv,iv>(psz, '&');
					for( auto param : params )
					{
						var keyValue = Str::Split<iv,iv>( param, '=' );
						if( keyValue.size()==2 && keyValue[0]=="query" )
						{
							SendQuery( ToSV(keyValue[1]), userId, move(req), s );
							break;
						}
					}
					handled = true;
				}
			}
			if( !handled )
				HandleRequest( move(target), move(params), move(pSessionInfo), move(req), move(s) );
		}
		catch( Exception& e )
		{
			s->Error( move(e), move(req) );
		}
	}

	α ISession::SendQuery( sv query, UserPK userId, http::request<http::string_body>&& req, sp<ISession> s )ι->Task
	{
		try
		{
			var result = ( co_await DB::CoQuery(string{query}, userId) ).UP<nlohmann::json>();
			s->Send( result->dump(), move(req) );
		}
		catch( Exception& e )
		{
			s->Error( move(e), move(req) );
		}
	}
}