#include <jde/app/client/await/SocketAwait.h>
#include <jde/framework/io/json.h>
#define let const auto

namespace Jde::App::Client{
	α GraphQLAwait::Execute( sp<AppClientSocketSession> pSession )ι->Web::Client::ClientSocketAwait<string>::Task{
		try{
			auto result = co_await pSession->GraphQL( move(_query), _userPK );
			Resume( Json::Parse(result) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α SessionInfoAwait::Execute( sp<AppClientSocketSession> pSession )ι->Web::Client::ClientSocketAwait<Proto::FromServer::SessionInfo>::Task{
		try{
			auto info = co_await pSession->SessionInfo( _sessionId );
			//let expiration = Chrono::ToClock<steady_clock,Clock>( IO::Proto::ToTimePoint(info.expiration()) );
			Resume( move(info) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}