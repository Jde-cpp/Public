#pragma once
#include "OpcServerSession.h"

namespace Jde::Opc::Gateway{
	struct CredentialAwait final : TAwait<Credential>{
		using base = TAwait<Credential>;
		CredentialAwait( SessionPK sessionId, ServerCnnctnNK opcId, SRCE )ι: base{sl}, _opcId{move(opcId)}, _sessionId{sessionId}{};
		α await_ready()ι->bool override;
		α Suspend()ι->void;
	private:
		ServerCnnctnNK _opcId;
		optional<Credential> _readyResult;
		SessionPK _sessionId;
	};
}
