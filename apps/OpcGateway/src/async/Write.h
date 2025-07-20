#pragma once
#include <jde/opc/uatypes/Value.h>

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct WriteAwait final : TAwait<flat_map<ExNodeId,UA_WriteResponse>>{
		WriteAwait( flat_map<ExNodeId,Value>&& x, sp<UAClient> c, SRCE )ι:TAwait{sl}, _values{move(x)},_client{move(c)}{}
		α Suspend()ι->void override;
	private:
		flat_map<ExNodeId,Value> _values;
		sp<UAClient> _client;
	};
	//Ξ SendRequest( flat_map<ExNodeId,Value>&& x, sp<UAClient> c, SRCE )ι->Await{ return Await{ move(x), move(c), sl }; }
	namespace Write{
		α OnResponse( UA_Client* client, void* userdata, RequestId requestId, UA_WriteResponse* response )ι->void;
	}
}