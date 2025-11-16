#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/web/server/Sessions.h>
#include "../types/ServerCnnctn.h"
#include "../auth/OpcServerSession.h"

namespace Jde::Opc::Gateway{
	struct UAClient; struct UAClientException;

	struct ConnectAwait final : TAwait<sp<UAClient>>, boost::noncopyable{
		using base = TAwait<sp<UAClient>>;
		ConnectAwait( ServerCnnctnNK&& opcTarget, Credential cred, SRCE )ι:base{sl},_opcTarget{move(opcTarget)}, _cred{move(cred)}{}
		ConnectAwait( ServerCnnctnNK opc, SessionPK sessionId, UserPK user, SRCE )ι;
		ConnectAwait( ServerCnnctnNK&& opc, const Web::Server::SessionInfo& session, SRCE )ι;
		α Suspend()ι->void override;
		α await_resume()ε->sp<UAClient> override{ return Promise() ? base::await_resume() : _result; }
		Ω Resume( sp<UAClient> client )ι->void;
		Ω Resume( str target, Credential cred, const UAClientException&& e )ι->void;
	private:
		Ω Resume( str target, Credential cred, function<void(ConnectAwait::Handle)> resume )ι->void;
		α Create()ι->TAwait<vector<ServerCnnctn>>::Task;
		Ω EraseRequests( str opcNK, Credential cred, lg& _ )ι->vector<ConnectAwait::Handle>;
		string _opcTarget;
		Credential _cred;
		sp<UAClient> _result;
	};
}