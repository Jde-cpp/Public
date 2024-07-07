#include <jde/appClient/AppClient.h>
#include "../../../Framework/source/io/ProtoUtilities.h"
#include "AppClientSocketSession.h"
#include "proto/App.FromClient.h"

#define var const auto

namespace Jde::App::Client{
}
namespace Jde::App{
	α Client::IsAppServer()ι->bool{ return false; }//TODO!
	function<vector<string>()> _statusDetails = []->vector<string>{ return {}; };
	α Client::SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void{ _statusDetails = f; }

	#define IF_OK if( auto pSession = IApplication::ShuttingDown() ? nullptr : AppClientSocketSession::Instance(); pSession )
	α Client::UpdateStatus()ι->void{
		IF_OK
			pSession->Write( FromClient::StatusMessage(_statusDetails()) );
	}

namespace Client{
	α SessionInfoAwait::await_suspend( base::Handle h )ι->void{
		base::await_suspend( h );
		if( auto pSession = AppClientSocketSession::Instance(); pSession ){
			[this,pSession,h]()->Http::ClientSocketAwait<Proto::FromServer::SessionInfo>::Task {
				try{
					auto info = co_await pSession->SessionInfo( _sessionId );
					var expiration = Chrono::ToClock<steady_clock,Clock>( IO::Proto::ToTimePoint(info.expiration()) );
					h.promise().Result = SessionInfo{ info.session_id(), expiration, info.user_pk(), info.user_endpoint(), info.has_socket() };
				}
				catch( IException& e ){
					h.promise().Exception = e.Move();
				}
				h.resume();
			}();
		}
		else
			h.resume();
	}
	α SessionInfoAwait::await_resume()ε->SessionInfo{
		base::AwaitResume();
		THROW_IF( !Promise(), "Shutting Down" );
		THROW_IF( !Promise()->Result, "No Connection to AppServer." );
		return *Promise()->Result;
	}
}}