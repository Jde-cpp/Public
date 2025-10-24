#pragma once
#include <jde/opc/uatypes/Value.h>

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct WriteResponse : UA_WriteResponse{
		WriteResponse( UA_WriteResponse&& x )ι:UA_WriteResponse{x}{ UA_WriteResponse_init(&x); }
		~WriteResponse(){ UA_WriteResponse_clear(this); }
	};
	struct WriteAwait final : TAwait<WriteResponse>{
		WriteAwait( NodeId nodeId, Value&& value, sp<UAClient> c, SRCE )ι:TAwait{sl}, _client{move(c)}, _nodeId{move(nodeId)}, _value{move(value)}{}
		α Suspend()ι->void override;
		α AddResponse( RequestId requestId, UA_WriteResponse&& response )ι->void;
	private:
		sp<UAClient> _client;
		NodeId _nodeId;
		RequestId _requestId{};
		Value _value;
	};
}