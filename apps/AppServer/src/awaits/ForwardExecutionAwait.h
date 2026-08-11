#pragma once
#include <jde/fwk/co/Await.h>

namespace Jde::App::Server{
	struct ServerSocketSession;
	struct ForwardExecutionAwait final: TAwait<string>{//kCustomResponse
		using base = TAwait<string>;
		ForwardExecutionAwait( UserPK userPK, Proto::FromClient::ForwardExecution&& customRequest, sp<ServerSocketSession> serverSocketSession, SRCE )ι;
		α Suspend()ι->void;
		Ω OnCloseConnection( uint32 requesterId, ConnectionPK targetConnectionPK )ι->void;//requesterId is the disconnecting session's socket Id() (unique, unlike ConnectionPK) - cancels forwards it asked for; targetConnectionPK cancels ones addressed to it.
		using base::ResumeExp;
		Ω Resume( string&& results, RequestId serverRequestId, ConnectionPK responder )ι->bool;
		Ω ResumeExp( Exception&& e, RequestId serverRequestId, ConnectionPK responder )ι->bool;

	private:
		UserPK _userPK;
		Proto::FromClient::ForwardExecution _forwardExecutionMessage;
		sp<ServerSocketSession> _requestSocketSession;
	};
}