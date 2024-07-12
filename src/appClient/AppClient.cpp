#include <jde/appClient/AppClient.h>
#include "../../../Framework/source/io/ProtoUtilities.h"
#include "AppClientSocketSession.h"
#include <jde/appClient/proto/App.FromClient.h>

#define var const auto

namespace Jde{
//	App::AppPK _appId;
//	App::AppInstancePK _instanceId;
	bool _isAppServer{false};

	α App::IsAppServer()ι->bool{ return _isAppServer; }
	α App::SetIsAppServer( bool x )ι->void{ _isAppServer = x; }
	// α App::AppId()ι->AppPK{ return _appId; }
	// α App::SetAppId( AppPK x )ι->void{ _appId=x; }
	// α App::InstanceId()ι->AppInstancePK{ return _instanceId;}
	// α App::SetInstanceId( AppInstancePK x )ι->void{ _instanceId=x; }
}

namespace Jde::App{

	function<vector<string>()> _statusDetails = []->vector<string>{ return {}; };
	α Client::SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void{ _statusDetails = f; }

	#define IF_OK if( auto pSession = IApplication::ShuttingDown() ? nullptr : AppClientSocketSession::Instance(); pSession )
	α Client::UpdateStatus()ι->void{
		IF_OK
			pSession->Write( FromClient::StatusMessage(_statusDetails()) );
	}

//	α Test( sp<Client::AppClientSocketSession> s )->Http::TTimedTask<Jde::App::Proto::FromServer::SessionInfo>{
//		Http::ClientSocketAwait<Proto::FromServer::SessionInfo> await = s->SessionInfo( 42 );
//coroutine_handle<Jde::Coroutine::TTask<Jde::App::Proto::FromServer::SessionInfo>::promise_type>’ to ‘
//coroutine_handle<Jde::Http::TTimedTask<Jde::App::Proto::FromServer::SessionInfo>::promise_type>
//		auto info = co_await await;
		//h.promise().SetValue( SessionInfo{info.session_id(), expiration, info.user_pk(), info.user_endpoint(), info.has_socket()} );
//	}
namespace Client{
	α SessionInfoAwait::await_suspend( base::Handle h )ι->void{
		base::await_suspend( h );
		if( auto pSession = AppClientSocketSession::Instance(); pSession ){
 			[this,pSession,h]()->Http::ClientSocketAwait<Proto::FromServer::SessionInfo>::Task {
// 			//Coroutine::TTask<Jde::App::Proto::FromServer::SessionInfo> { --works
// 			//Http::TTimedTask<Jde::App::Proto::FromServer::SessionInfo> { --error
// 			//
// //				Jde::Coroutine::TTask<Jde::App::Proto::FromServer::SessionInfo>’ to
// //				‘Jde::Coroutine::VoidAwait<Jde::App::Proto::FromServer::SessionInfo, Jde::Http::TTimedTask<Jde::App::Proto::FromServer::SessionInfo> >::Task’ {
// //					aka ‘Jde::Http::TTimedTask<Jde::App::Proto::FromServer::SessionInfo>’
 				try{
// 					Http::ClientSocketAwait<Proto::FromServer::SessionInfo> await = pSession->SessionInfo( _sessionId );
// 					auto info = co_await await;
 					auto info = co_await pSession->SessionInfo( _sessionId );
 					var expiration = Chrono::ToClock<steady_clock,Clock>( IO::Proto::ToTimePoint(info.expiration()) );
 					h.promise().SetValue( Web::SessionInfo{info.session_id(), expiration, info.user_pk(), info.user_endpoint(), info.has_socket()} );
 				}
 				catch( IException& e ){
 					h.promise().SetError( e.Move() );
 				}
 				h.resume();
 			}();
		}
		else
			h.resume();
	}
	α SessionInfoAwait::await_resume()ε->Web::SessionInfo{
		base::AwaitResume();
		THROW_IF( !Promise(), "Shutting Down" );
		THROW_IF( !Promise()->Value(), "No Connection to AppServer." );
		return *Promise()->Value();
	}
}}