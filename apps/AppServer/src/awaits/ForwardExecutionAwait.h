#pragma once
#include <jde/fwk/co/Await.h>

namespace Jde::App::Server{
	struct ServerSocketSession;
	struct ForwardExecutionAwait final: TAwait<string>{//kCustomResponse
		using base = TAwait<string>;
		ForwardExecutionAwait( UserPK userPK, Proto::FromClient::ForwardExecution&& customRequest, sp<ServerSocketSession> serverSocketSession, SRCE )ι;
		α Suspend()ι->void;
		Ω OnCloseConnection( ProgInstPK instancePK )ι->void;
		using base::ResumeExp;
		Ω Resume( string&& results, RequestId serverRequestId )ι->bool;
		Ω ResumeExp( Exception&& e, RequestId serverRequestId )ι->bool;

	private:
		UserPK _userPK;
		Proto::FromClient::ForwardExecution _forwardExecutionMessage;
		sp<ServerSocketSession> _requestSocketSession;
	};
}