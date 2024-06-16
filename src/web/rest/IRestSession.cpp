#include <jde/web/rest/IRestSession.h>
#include "../../../../Framework/source/math/MathUtilities.h"
#include <jde/web/rest/RestException.h>

#define var const auto
#define CHECK_EC(ec, ...) if( ec ){ CodeException x(static_cast<std::error_code>(ec) __VA_OPT__(,) __VA_ARGS__); return; }

namespace Jde::Web::Rest{
	static sp<LogTag> _logTag = Logging::Tag( "restRequest" );
	static sp<LogTag> _logTagResponse = Logging::Tag( "restResponse" );
	α RequestTag()ι->sp<LogTag>{ return _logTag; }
	α ResponseTag()ι->sp<LogTag>{ return _logTagResponse; }

	α IRestSession::DoRead()ι->void{
    _stream.expires_after( std::chrono::seconds(30) );
    http::async_read( _stream, _buffer, _request, beast::bind_front_handler( &IRestSession::OnRead, MakeShared()) );
  }

  α IRestSession::OnRead( beast::error_code ec, std::size_t bytes_transferred )ι->void{
    boost::ignore_unused( bytes_transferred );
    if( ec == http::error::end_of_stream )
      return DoClose();
		CHECK_EC( ec );
		HandleRequest( Request{MakeShared()} );
  }

	α IRestSession::OnWrite( bool /*close*/, beast::error_code ec, std::size_t bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), ec.value()==(int)boost::beast::error::timeout ? ELogLevel::Debug : ELogLevel::Error };
		DoClose();
  }

	α IRestSession::DoClose()ι->void{
		beast::error_code ec;
		_stream.socket().shutdown(tcp::socket::shutdown_send, ec);
  }

	Ŧ SetResponse( http::response<T>& res, bool keepAlive )ι->void{
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::access_control_allow_origin, "*");
		res.keep_alive( keepAlive );
    res.prepare_payload();
	}

	Ŧ SetBodyResponse( http::response<T>& res, bool keepAlive )ι->void{
		res.set( http::field::content_type, "application/json" );
		SetResponse<T>( res, keepAlive );
	}

	α IRestSession::Send( string&& value, Request&& req )ι->void{
		auto res = ms<http::response<http::string_body>>( std::piecewise_construct, std::make_tuple(std::move(value)), std::make_tuple(http::status::ok, req.ClientRequest().version()) );
		SetBodyResponse( *res, req.ClientRequest().keep_alive() );
		Send( move(res), move(req.Session) );
	}

	α IRestSession::SendOptions( Request&& req )ι->void{
		auto y = ms<http::response<http::empty_body>>( http::status::no_content, req.ClientRequest().version() );
    y->set(http::field::access_control_allow_methods, "GET, POST");
    y->set(http::field::access_control_allow_headers, "*");
    y->set(http::field::access_control_max_age, "86400");
		SetResponse( *y, req.ClientRequest().keep_alive() );
    Send( move(y), move(req.Session) );
	}

	α IRestSession::Send( IException&& e, Request&& req )ι->void{
		var p = dynamic_cast<IRestException*>( &e );
		string what = p ? p->what() : Jde::format( "Internal Server Error:  '{:x}'.", e.Code );
		e.Move();  //want the log to show here.
		Send( p ? p->Status() : http::status::internal_server_error, what, move(req) );
	}

	α IRestSession::Send( const IRestException&& e, Request&& req )ι->void{
		Send( e.Status(), e.what(), move(req) );
	}

	α IRestSession::Send( http::status status, string what, Request&& req )ι->void{
    auto y = ms<http::response<http::string_body>>( status, req.ClientRequest().version() );
		var message = Jde::format( "{{\"message\": \"{}\"}}", move(what) );
		Dbg( message, _logTag );
		SetBodyResponse( *y, req.ClientRequest().keep_alive() );
    y->body() = message;
    y->prepare_payload();
		Send( move(y), move(req.Session) );
	}

	static flat_map<SessionPK, sp<SessionInfo>> _sessions; shared_mutex _sessionMutex;
	α AddSession2( sp<SessionInfo>& p )ι->void{
		ul _{ _sessionMutex };
		_sessions.emplace( p->session_id(), p );
	}
	α IRestSession::QuerySessions( SessionPK sessionId )ι->vector<sp<SessionInfo>>{
		ul _{ _sessionMutex };
		vector<sp<SessionInfo>> sessions;
		if( sessionId ){
			if( auto p = _sessions.find(sessionId); p!=_sessions.end() )
				sessions.push_back( p->second );
		}
		else{
			sessions.reserve( sessionId ? 1 : _sessions.size() );
			for( auto& [_,p] : _sessions )
				sessions.push_back( p );
		}
		return sessions;
	}
	
	α IRestSession::GetNewSessionId()ι->SessionPK{
		ul _{ _sessionMutex };
		auto sessionId{ Math::Random() };
		while( _sessions.contains(sessionId) )
			sessionId = Math::Random();
		return sessionId;
	}

	α IRestSession::AddSession( UserPK userId )ι->sp<SessionInfo>{
		auto p = ms<SessionInfo>();
		auto pTimestamp = mu<Logging::Proto::Timestamp>();
		pTimestamp->set_seconds( time(nullptr)+60*60 );
		p->set_allocated_expiration( pTimestamp.release() );
		p->set_session_id( GetNewSessionId() );
		p->set_user_id( userId );
		AddSession2( p );
		return p;
	}

	α FetchSessionInfo( SessionPK sessionId, tcp::endpoint userEndpoint, HCoroutine h )ι->Task{
		try{
			sp<SessionInfo> p{ (co_await Logging::Server::FetchSessionInfo(sessionId, userEndpoint)).UP<SessionInfo>().release() };
			AddSession2(p);
			h.promise().SetResult<SessionInfo>( move(p) );
		}
		catch( IException& e ){
			h.promise().SetResult( move(e) );
		}
		h.resume();
	}

	α SessionInfoAwait::await_ready()ι->bool{
		sl _{ _sessionMutex };
		if( auto p = _sessions.find( _sessionId ); p!=_sessions.end() )
			_result.Set( p->second );
		else if( Logging::Server::IsLocal() )//if local, should have been in _sessions, ie server reset
			_result.Set( sp<SessionInfo>{} );
		return Logging::Server::IsLocal() || _result.HasShared();
	}
	α SessionInfoAwait::await_suspend( HCoroutine h )ι->void{ IAwaitCache::await_suspend(h); FetchSessionInfo( _sessionId, _userEndpoint, h ); }

	α Request::Body()Ε->json{
		return Json::Parse( ClientRequest().body() );
	}

	α ParseUri( string&& uri )->tuple<string,flat_map<string,string>>{
	  var target{ uri.substr(0, uri.find('?')) };
		flat_map<string,string> params;
		if( target.size()+1<uri.size() ){
			var start = target.size()+1;
			sv paramString = sv{ uri.data()+start, uri.size()-start };
			var paramStringSplit = Str::Split( paramString, '&' );
			for( auto param : paramStringSplit ){
				var keyValue = Str::Split( param, '=' );
				params[string{keyValue[0]}]=keyValue.size()==2 ? string{keyValue[1]} : string{};
			}
		}
		return make_tuple( target, params );
	}
	α IRestSession::HandleRequest( Request req )ι->Task{
		auto [target, params] = ParseUri( Ssl::DecodeUri(string{req.ClientRequest().target()}) );
		TRACE( "{}={}", target, req.ClientRequest().body() );
		var sessionString{ req.ClientRequest().base()["Session-Id"] };
		try{
			if( !sessionString.empty() ){
				var sessionId = Str::TryTo<SessionPK>( string{sessionString}, nullptr, 16 ); THROW_IFX(!sessionId, RestException<http::status::bad_request>(SRCE_CUR, "Could not create sessionId {}.", sessionString); )
				req.SessionInfoPtr = ( co_await FetchSessionInfo(*sessionId, {}) ).UP<SessionInfo>();
				if( req.SessionInfoPtr && req.SessionInfoPtr->expiration().seconds()<time(nullptr) )
					RestException<http::status::unauthorized>( SRCE_CUR, "session timeout '{}'.", sessionString);
			}
	    if( req.Method() == http::verb::get ){
				if( target=="/graphql" ){
					auto& query = params["query"]; THROW_IFX( query.empty(), RestException<http::status::bad_request>(SRCE_CUR, "No query sent.") );
					var sessionId = req.SessionInfoPtr ? req.SessionInfoPtr->session_id() : 0;
					TRACE( "[{:x}] - {}", sessionId, query );
					string threadDesc = Jde::format( "[{:x}]{}", sessionId, target );
					var y = await( json, DB::CoQuery(move(query), req.UserId(), threadDesc) );
					TRACET( _logTagResponse, "[{:x}] - {}", sessionId, y.dump() );
					Send( move(y), move(req) );
				}
			}
			else if( req.Method() == boost::beast::http::verb::options )
				SendOptions( move(req) );

			if( req.Session )
				HandleRequest( move(target), move(params), move(req) );
			if( req.Session )
				throw RestException<http::status::not_found>( SRCE_CUR, "Could not find target '{}'.", target );
		}
		catch( IException& e ){
			Send( move(e), move(req) );
		}
	}

	α IRestSession::Send( const json& payload, Request&& req )ι->void{
		Send( payload.dump(), move(req) );
	}
}