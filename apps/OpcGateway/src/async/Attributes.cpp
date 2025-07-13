#include "Attributes.h"
#include "../UAClient.h"
#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };

	α AttribAwait::Suspend()ι->void{ _client->RequestDataTypeAttributes( move(_values), _h ); }

namespace Attributes{
	α OnResponse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode /*status*/, UA_NodeId* dataType )ι->void{
		let handle = userdata ? (RequestId)(uint)userdata : requestId;
		string logPrefix = format( "[{:x}.{}.{}]", (uint)ua, handle, requestId );
		auto ppClient = Try<sp<UAClient>>( [ua](){return UAClient::Find(ua);} ); if( !ppClient ) return;
		optional<flat_map<ExNodeId, ExNodeId>> results;
		bool visited = (*ppClient)->_dataAttributeRequests.visit( handle, [requestId, dataType, &results]( auto& pair ){
			auto& request = pair.second;
			if( auto pRequest=request.Requests.find(requestId); pRequest!=request.Requests.end() ){
				request.Results.try_emplace( pRequest->second, dataType ? move(*dataType) : ExNodeId{} );
				if( request.Results.size()==request.Requests.size() )
					results = move( request.Results );
			}
		});
		if( !visited )
			CRITICAL( "{}Could not find handle.", logPrefix );
		else if( results ){
			(*ppClient)->_dataAttributeRequests.erase( handle );
			auto h = (*ppClient)->ClearRequestH<AttribAwait::Handle>( handle ); RETURN_IF( !h, ELogLevel::Critical, "[{:x}.{:x}]Could not find handle.", (uint)ua, requestId );
			h.promise().Resume( move(*results), h );
		}
	}
}}