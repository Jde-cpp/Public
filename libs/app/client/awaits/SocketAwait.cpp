#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/fwk/io/json.h>

#define let const auto

namespace Jde::App::Client{
	α SessionInfoAwait::Execute()ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task{
		try{
			Web::FromServer::SessionInfo info;
			info = co_await _session->SessionInfo( _credentials );
			Resume( move(info) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}