#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>

namespace Jde::Iot{
	struct ΓI AuthenticateAwait : TAwaitEx<App::Proto::FromServer::SessionInfo,Jde::Task>{
		using base = TAwaitEx<App::Proto::FromServer::SessionInfo,Jde::Task>;
		AuthenticateAwait( str loginName, str password, str opcNK, str endpoint, bool isSocket, SRCE )ι;
		α Execute()ι->Jde::Task override;
	private:
		string _loginName; string _password; string _opcNK; string _endpoint; bool _isSocket;
	};
	//CRD - Insert/Purge/Select from um_providers table
	struct ΓI ProviderAwait : AsyncAwait{
		ProviderAwait( str opcId, SRCE )ι;//select
		ProviderAwait( variant<uint32,string> opcIdTarget, bool insert, SRCE )ι;//insert/purge
	};
	ΓI α Credentials( SessionPK sessionId, str opcId )ι->tuple<string,string>;

}