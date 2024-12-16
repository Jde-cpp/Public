#pragma once
#include <jde/opc/uatypes/UAClient.h>
#include <jde/opc/uatypes/Value.h>

namespace Jde::Opc::Write{
	struct Await final : IAwait
	{
		Await( flat_map<NodeId,Value>&& x, sp<UAClient>&& c, SRCE )ι:IAwait{sl}, _values{move(x)},_client{move(c)}{}
		α Suspend()ι->void override{ _client->SendWriteRequest( move(_values), _h ); }
	private:
		flat_map<NodeId,Value> _values;
		sp<UAClient> _client;
	};
	Ξ SendRequest( flat_map<NodeId,Value>&& x, sp<UAClient> c, SRCE )ι->Await{ return Await{ move(x), move(c), sl }; }
	α OnResonse( UA_Client *client, void *userdata, RequestId requestId, UA_WriteResponse *response )ι->void;
}