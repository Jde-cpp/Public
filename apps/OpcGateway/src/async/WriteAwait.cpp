#include "WriteAwait.h"
#include <jde/fwk/exceptions/ArgException.h>
#include "../UAClient.h"
#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	//concurrent_flat_map<Jde::Handle, UARequestMulti<UA_WriteResponse>> _writeRequests;

	α onResponse( UA_Client*, void* userdata, RequestId requestId, UA_WriteResponse* response )ι->void{
		auto& await = *(WriteAwait*)userdata;
		await.AddResponse( requestId, move(*response) );
	}
	α WriteAwait::Suspend()ι->void{
		UAε( UA_Client_writeValueAttribute_async(*_client, _nodeId, &_value.value, onResponse, this, &_requestId) );
		_client->Process( _requestId );
	}
	α WriteAwait::AddResponse( RequestId requestId, UA_WriteResponse&& response )ι->void{
		_client->ClearRequest( requestId );
		Resume(move(response));
	}
}