#include "WriteAwait.h"
#include <jde/fwk/exceptions/ArgException.h>
#include "../UAClient.h"
#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	//concurrent_flat_map<Jde::Handle, UARequestMulti<UA_WriteResponse>> _writeRequests;

	α WriteResponse::operator=( WriteResponse&& x )ι->WriteResponse&{
		if( this != &x ){
			UA_WriteResponse_clear(this);
			*(UA_WriteResponse*)this = x;
			UA_WriteResponse_init( &x );
		}
		return *this;
	}
	α onResponse( UA_Client*, void* userdata, RequestId requestId, UA_WriteResponse* response )ι->void{
		auto& await = *(WriteAwait*)userdata;
		await.AddResponse( requestId, move(*response) );
	}
	α WriteAwait::Suspend()ι->void{
		UAε( UA_Client_writeValueAttribute_async(*_client, _nodeId, &_value.value, onResponse, this, &_requestId) );
		_client->Process( _requestId, "writeValueAttribute" );
	}
	α WriteAwait::AddResponse( RequestId reqId, UA_WriteResponse&& response )ι->void{
		_client->ClearRequest( reqId );
		if( response.responseHeader.serviceResult )
			ResumeExp( UAClientException{response.responseHeader.serviceResult, _client->Handle(), "writeValueAttribute", _sl} );
		else if( response.resultsSize && *response.results )
			ResumeExp( UAClientException{response.results[0], _client->Handle(), "writeValueAttribute", _sl} );
		else
			Resume(move(response));
	}
}