#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/framework/io/json.h>
#define let const auto

namespace Jde::App::Client{
/*	α GraphQLAwait::Execute( sp<AppClientSocketSession> pSession )ι->Web::Client::ClientSocketAwait<string>::Task{
		try{
			auto result = co_await pSession->Query( move(_query) );
			Resume( Json::Parse(result) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
*/
	α SessionInfoAwait::Execute( sp<AppClientSocketSession> pSession )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task{
		try{
			auto info = co_await pSession->SessionInfo( _sessionId );
			//let expiration = Chrono::ToClock<steady_clock,Clock>( Proto::ToTimePoint(info.expiration()) );
			Resume( move(info) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}