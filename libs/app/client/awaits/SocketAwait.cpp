#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/fwk/io/json.h>
#include <jde/app/proto/app.FromClient.h>

#define let const auto

namespace Jde::App::Client{
	α SessionInfoAwait::Execute()ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task{
		try{
			Web::FromServer::SessionInfo info;
			info = co_await _session->SessionInfo( _credentials );
			Resume( move(info) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AddSessionAwait::Execute()ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task{
		try{
			let requestId = _session->NextRequestId();
			TRACET( ELogTags::SocketClientWrite, "AddSession domain: '{}', loginName: '{}', providerPK: {}, userEndPoint: '{}', isSocket: {}.", _domain, _loginName, _providerPK, _userEndPoint, _isSocket );
			auto info = co_await Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>{ FromClient::AddSession(_domain, _loginName, _providerPK, _userEndPoint, _isSocket, requestId), requestId, _session, _sl };
			Resume( move(info) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}