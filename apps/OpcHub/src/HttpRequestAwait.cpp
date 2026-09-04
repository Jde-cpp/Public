#include "HttpRequestAwait.h"
#include <jde/web/server/Sessions.h>
#include "../../AppServer/src/HttpRequestAwait.h"
#include "../../AppServer/src/WebServer.h"
#include "../../OpcGateway/src/auth/OpcServerSession.h"
#define let const auto

namespace Jde::Opc::Hub{
	α HttpRequestAwait::Schemas()Ι->const vector<sp<DB::AppSchema>>&{ return App::Server::Schemas(); }

	α HttpRequestAwait::await_ready()ι->bool{
		if( _request.Method()==http::verb::get ){
			if( _request.Target()=="/GoogleAuthClientId" ){
				_request.LogRead();
				_readyResult = mu<jvalue>( App::Server::Routes::GoogleAuthClientId() );
			}
			else if( _request.Target()=="/opcGateways" || _request.Target()=="/opcServers" ){
				_request.LogRead();
				_readyResult = mu<jvalue>( App::Server::Routes::Instances(_request.Target()=="/opcServers", _request.UserPK()!=Jde::UserPK{}) );
			}
		}
		return _readyResult!=nullptr || base::await_ready();
	}
	α HttpRequestAwait::HasOpcBody()ι->bool{
		try{
			return _request.Body().contains( "opc" );
		}
		catch( const std::exception& ){//not json - the app's login carries no body.
			return false;
		}
	}
	α HttpRequestAwait::HubLogout()ι->void{
		try{
			_request.LogRead();
			let sessionId = _request.SessionId();
			Gateway::Logout( sessionId );//the OPC credentials cached for the session.
			jobject j{ {"removed", Web::Server::Sessions::Remove(sessionId)} };//and every socket bound to it, on either path.
			Resume( {move(j), move(_request)} );
		}
		catch( runtime_error& e ){
			ResumeExp( RestException{EHttpStatus::InternalServerError, move(e), move(_request)} );
		}
	}
	α HttpRequestAwait::Suspend()ι->void{
		if( _request.IsPost("/login") ){
			if( _request.Header("Authorization").starts_with("Bearer ") )
				App::Server::Routes::LoginJwt( _request, _h );
			else if( HasOpcBody() )
				Login( _request.UserEndpoint.address().to_string() );
			else
				ResumeExp( RestException{EHttpStatus::Unauthorized, SRCE_CUR, move(_request), "Missing or invalid Authorization header"} );
		}
		else if( _request.IsPost("/logout") )
			HubLogout();
		else
			base::Suspend();//?opc= and the 404.
	}
}
