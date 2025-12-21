#include "HttpRequestAwait.h"
#include <jde/fwk/process/execution.h>
#include <jde/web/server/auth/JwtLoginAwait.h>
#include "LocalClient.h"
#include "WebServer.h"
#include "types/rest/json.h"
#define let const auto

namespace Jde::App{
	HttpRequestAwait::HttpRequestAwait( HttpRequest&& req, SL sl )ι:
		base{ move(req), sl }
	{}

	α ValueJson( string&& value )ι{ return Json::Parse( Ƒ("{{\"value\": \"{}\"}}", value) ); }

	Ω login( HttpRequest req, HttpRequestAwait::Handle h )ι->TAwait<UserPK>::Task{
		try{
			req.LogRead();
			let authorization = req.Header("Authorization");
			THROW_IFX( authorization.empty() || !authorization.starts_with("Bearer "), RestException<http::status::unauthorized>(SRCE_CUR, move(req), "Missing or invalid Authorization header") );

			req.SessionInfo->UserPK = co_await JwtLoginAwait( Web::Jwt{authorization.substr(7)}, req.UserEndpoint.address().to_string(), Server::AppClient() );
			jobject j{ {"expiration", ToIsoString(req.SessionInfo->Expiration)} };
			req.SessionInfo->IsInitialRequest = true;  //expecting sessionId to be set.
			h.promise().Resume( {move(j), move(req)}, h );
		}
		catch( exception& e ){
			h.promise().ResumeExp( move(e), h );
		}
	}

	α Logout( HttpRequest&& req, HttpRequestAwait::Handle h )ι->void{
		try{
			req.LogRead();
			jobject j{ {"removed", Sessions::Remove(req.SessionInfo->SessionId)} };
			h.promise().SetValue( {move(j), move(req)} );
		}
		catch( IException& e ){
			h.promise().SetExp( move(e) );
		}
		h.resume();
	}

	α HttpRequestAwait::await_ready()ι->bool{
		if( _request.Method() == http::verb::get ){
			if( _request.Target()=="/GoogleAuthClientId" ){
				_request.LogRead();
				_readyResult = mu<jvalue>( ValueJson(Settings::FindString("GoogleAuthClientId").value_or("GoogleAuthClientId Not Configured.")) );
			}
			else if( _request.Target()=="/opcGateways" || _request.Target()=="/opcServers" ){
				_request.LogRead();
				let apps = Server::FindApplications( _request.Target()=="/opcServers" ? "Jde.OpcServer" : "Jde.OpcGateway" );
				jarray japps;
				for( auto& app : apps )
					japps.push_back( ToJson(app) );
				_readyResult = mu<jvalue>( jobject{{"servers", japps}} );
			}
		}
		return _readyResult!=nullptr;
	}
	α HttpRequestAwait::Suspend()ι->void{
		up<IException> pException;
		bool processed{ _request.Method() == http::verb::post };
		if( _request.Method() == http::verb::post ){
			if( _request.Target()=="/login" )
				login( move(_request), _h );
			else if( _request.Target()=="/logout" )
				Logout( move(_request), _h );
			else
				processed = false;
		}
		if( !processed ){
			_request.LogRead();
			RestException<http::status::not_found> e{ SRCE_CUR, move(_request), "Unknown target '{}'", _request.Target() };
			ResumeExp( RestException<http::status::not_found>(move(e)) );
		}
	}

	α HttpRequestAwait::await_resume()ε->HttpTaskResult{
		if( auto e = Promise() ? Promise()->MoveExp() : nullptr; e ){
			if( auto rest = dynamic_cast<IRestException*>( e.get() ); rest )
				rest->Throw();
			else
				throw RestException<http::status::internal_server_error>{ move(*e), move(_request) };
		}
		return _readyResult
			? HttpTaskResult{ move(*_readyResult), move(_request) }
			: Promise()->Value() ? move( *Promise()->Value() ) : HttpTaskResult{ {}, move(_request) };
	}
}