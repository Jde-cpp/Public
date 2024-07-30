#include <jde/app/client/await/SocketAwait.h>
#include <jde/io/Json.h>
#define var const auto

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
			var expiration = Chrono::ToClock<steady_clock,Clock>( IO::Proto::ToTimePoint(info.expiration()) );
			Resume( Web::Server::SessionInfo{info.session_id(), expiration, info.user_pk(), info.user_endpoint(), info.has_socket()} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}