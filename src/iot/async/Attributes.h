﻿#pragma once
#include <jde/iot/uatypes/UAClient.h>

namespace Jde::Iot{ struct NodeId; }

namespace Jde::Iot::Attributes{
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