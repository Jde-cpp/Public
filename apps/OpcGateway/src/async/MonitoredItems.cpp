#include "MonitoredItems.h"
#include <open62541/client_subscriptions.h>
#include "../UAClient.h"
#include "../types/UAClientException.h"
#include "../usings.h"

#define let const auto

namespace Jde::Opc::Gateway{
	DeleteMonitoredItemsAwait::DeleteMonitoredItemsAwait( flat_map<SubscriptionId,flat_set<MonitorId>> monitoredItems, sp<UAClient> client, SL sl )ι:
		VoidAwait{sl},
		_client{ move(client) },
		_monitoredItems{ move(monitoredItems) }
	{}

	Ω monitoredItemsDeleteCallback( UA_Client*, void* userdata, RequestId requestId, UA_DeleteMonitoredItemsResponse* response )ι->void{
		DeleteMonitoredItemsAwait& await = *(DeleteMonitoredItemsAwait*)userdata;
		await.OnComplete( *response, requestId );
	}

	α DeleteMonitoredItemsAwait::Suspend()ι->void{
		bool first{true};
		try{
			for( let& [subscriptionId, monitorIds] : _monitoredItems ){
				UA_DeleteMonitoredItemsRequest request{
					.subscriptionId = subscriptionId,
					.monitoredItemIdsSize = monitorIds.size(),
					.monitoredItemIds = ( UA_UInt32* )UA_Array_new( monitorIds.size(), &UA_TYPES[UA_TYPES_UINT32] )
				};
				uint i=0;
				for( let& id : monitorIds )
					request.monitoredItemIds[i++] = id;

				RequestId _requestId{};
				UACε( UA_Client_MonitoredItems_delete_async(_client->UAPointer(), request, monitoredItemsDeleteCallback, this, &_requestId) );
				TRACE( "[{}.{}]Deleting monitored items for subscriptionId={}, count={}", _client->Handle(), _requestId, subscriptionId, monitorIds.size() );
				if( first ){
					_client->Process( _requestId, nullptr, "MonitoredItems_delete" );
					first = false;
				}
				UA_DeleteMonitoredItemsRequest_clear( &request );
			}
		}
		catch( IException& e ){
			if( first )
				ResumeExp( move(e) );
		}
	}
	α DeleteMonitoredItemsAwait::OnComplete( UA_DeleteMonitoredItemsResponse& response, RequestId requestId )ι->void{
		_client->ClearRequest( requestId );
		TRACE( "[{}.{}]MonitoredItemsDeleteCallback", _client->Handle(), requestId );
		if( let sc = response.responseHeader.serviceResult; sc )
			WARN( "[{}.{}]Could not delete monitored items:  {}.", _client->Handle(), requestId, UAException::Message(sc) );
    for( auto sc : Iterable<UA_StatusCode>(response.results, response.resultsSize) ){
			if( sc )
				WARN( "[{}.{}]Could not delete monitored item:  {}.", _client->Handle(), requestId, UAException::Message(sc) );
		}
		UA_DeleteMonitoredItemsResponse_clear( &response );
		_finished.emplace( requestId );
		if( _finished.size()==_monitoredItems.size() )
			Resume();
	}
}