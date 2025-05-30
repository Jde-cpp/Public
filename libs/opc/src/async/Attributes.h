#pragma once
#include <jde/opc/uatypes/UAClient.h>

namespace Jde::Opc{ struct NodeId; }

namespace Jde::Opc::Attributes{
	struct Await final : IAwait{
		Await( flat_set<NodeId>&& x, sp<UAClient>&& c, SRCE )ι:IAwait{sl}, _values{move(x)},_client{move(c)}{}
		α Suspend()ι->void override{ _client->RequestDataTypeAttributes( move(_values), _h ); }
	private:
		flat_set<NodeId> _values;
		sp<UAClient> _client;
	};
	Ξ ReadDataTypeAttributes( flat_set<NodeId>&& x, sp<UAClient> c, SRCE )ι->Await{ return Await{ move(x), move(c), sl }; }
	α OnResonse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_NodeId *dataType )ι->void;
}