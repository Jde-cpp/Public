#include "Write.h"
#include "../UAClient.h"
#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	α WriteAwait::Suspend()ι->void{ _client->SendWriteRequest( move(_values), _h ); }

	α Write::OnResponse( UA_Client* ua, void* userdata, RequestId requestId, UA_WriteResponse* response )ι->void{
		let handle = userdata ? (RequestId)(uint)userdata : requestId;
		string logPrefix = format( "[{:x}.{}.{}]", (uint)ua, handle, requestId );
		DBG( "[{}]Write::OnResponse()", logPrefix );
		auto ppClient = Try<sp<UAClient>>( [ua](){return UAClient::Find(ua);} ); if( !ppClient ) return;
		optional<flat_map<ExNodeId, UA_WriteResponse>> results;
		let visited = (*ppClient)->_writeRequests.visit( handle, [requestId, response, &results]( auto& pair ){
			auto& x = pair.second;
			if( auto pRequest=x.Requests.find(requestId); pRequest!=x.Requests.end() ){
				x.Results.try_emplace( pRequest->second, *response ); memset( response, 0, sizeof(UA_WriteResponse) );
				if( x.Results.size()==x.Requests.size() )
					results = move( x.Results );
			}
		});
		if( !visited )
			CRITICAL( "{}Could not find handle.", logPrefix );
		else if( results ){
			(*ppClient)->_readRequests.erase( handle );
			auto h = (*ppClient)->ClearRequestH<TAwait<flat_map<ExNodeId,UA_WriteResponse>>::Handle>( handle ); RETURN_IF( !h, ELogLevel::Critical, "[{:x}.{:x}]Could not find handle.", (uint)ua, requestId );
			h.promise().Resume( move(*results), h );
		}
		DBG( "[{}]Value::~OnResponse()", logPrefix );
	}
}