#include "Attributes.h"
#define var const auto

namespace Jde::Iot::Attributes{
	var _logTag = Logging::Tag( "app.attributes" );
	α OnResonse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode status, UA_NodeId* dataType )ι->void{
		var handle = userdata ? (RequestId)(uint)userdata : requestId;
		string logPrefix = format( "[{:x}.{}.{}]", (uint)ua, handle, requestId );
		auto ppClient = Try<sp<UAClient>>( [ua](){return UAClient::Find(ua);} ); if( !ppClient ) return;
		up<flat_map<NodeId, NodeId>> results;
		bool visited = (*ppClient)->_dataAttributeRequests.visit( handle, [requestId, dataType, &results]( auto& pair ){
			auto& request = pair.second;
			if( auto pRequest=request.Requests.find(requestId); pRequest!=request.Requests.end() ){
				request.Results.try_emplace( pRequest->second, dataType ? move(*dataType) : NodeId{} );
				if( request.Results.size()==request.Requests.size() )
					results = mu<flat_map<NodeId, NodeId>>( move(request.Results) );
			}
		});
		if( !visited )
			CRITICAL( "{}Could not find handle.", logPrefix );
		else if( results ){
			(*ppClient)->_dataAttributeRequests.erase( handle );
			HCoroutine h = (*ppClient)->ClearRequestH( handle ); RETURN_IF( !h, ELogLevel::Critical, "[{:x}.{:x}]Could not find handle.", (uint)ua, requestId );
			Resume( move(results), move(h) );
		}
	}
}