#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/access/usings.h>
#include <jde/web/server/Web.FromServer.h>

namespace Jde::App::Server{
	//ServerSocketSession::AddSession without the socket: authenticate, mint a web session, hand back its proto.  For an
	//in-process app client (OpcHub's gateway) whose OPC user/password login used to travel the kAddSession round trip.
	struct AddSessionAwait final : TAwait<Web::FromServer::SessionInfo>{
		AddSessionAwait( string domain, string loginName, Access::ProviderPK providerPK, string userEndPoint, bool isSocket, SRCE )ι:
			TAwait<Web::FromServer::SessionInfo>{sl}, _domain{move(domain)}, _loginName{move(loginName)}, _userEndPoint{move(userEndPoint)}, _providerPK{providerPK}, _isSocket{isSocket}{}
		α Suspend()ι->void override{ Execute(); }
	private:
		α Execute()ι->TAwait<Jde::UserPK>::Task;
		string _domain, _loginName, _userEndPoint;
		Access::ProviderPK _providerPK;
		bool _isSocket;
	};
}
