#include "DataChanges.h"
#include <jde/opc/uatypes/Value.h>
#include "../UAClient.h"
#include "../uatypes/CreateMonitoredItemsRequest.h"

#define let const auto

namespace Jde::Opc::Gateway{
	static ELogTags _tags{ (ELogTags)(EOpcLogTags::Opc | EOpcLogTags::Monitoring) };
	Ω createDataChangesCallback( UA_Client*, void *userdata, RequestId, UA_CreateMonitoredItemsResponse* response )ι->void{
		DataChangeAwait& await = *(DataChangeAwait*)userdata;
		await.OnComplete( response );
	}

	Ω dataChangesCallback( UA_Client* ua, SubscriptionId subId, void* /*subContext*/, MonitorId monId, void* /*monContext*/, UA_DataValue* uaValue )->void{
		auto pClient = UAClient::TryFind(ua); if(!pClient) return;
		Value value{ move(*uaValue) };
		let h = MonitorHandle{ subId, monId };
		TRACET( DataChangesTag, "[{:x}.{:x}] DataChangesCallback - {}", (uint)ua, (Handle)h, serialize(value.ToJson()) );
		if( !pClient->MonitoredNodes().SendDataChange(h, move(value)) )
			DBGT( DataChangesTag, "[{:x}.{:x}]Could not find node monitored item.", (uint)ua, (Handle)MonitorHandle{subId, monId} );
	}

	Ω dataChangesDeleteCallback( UA_Client* ua, SubscriptionId subId, void* /*_subContext_*/, MonitorId monId, void* /*_monContext_*/ )->void{
		TRACE( "[{:x}.{:x}]DataChangesDeleteCallback", (uint)ua, (Handle)MonitorHandle{subId, monId} );
	}

	α DataChangeAwait::Suspend()ι->void{
		auto subscription = _client->CreatedSubscriptionResponse;
		if( !subscription ){
			ResumeExp( Exception{"CreatedSubscriptionResponse==null"} );
			return;
		}
		auto request = _client->MonitoredNodes().MonitoredItemsRequest( move(_dataChange), move(_nodes), _monitoredRequestId );
		if( !request ){
			_h.resume();
			return;
		}
		try{
			vector<UA_Client_DeleteMonitoredItemCallback> deleteCallbacks{ request->itemsToCreateSize, dataChangesDeleteCallback };
			vector<UA_Client_DataChangeNotificationCallback> dataChangeCallbacks{ request->itemsToCreateSize, dataChangesCallback };
			void** contexts = nullptr;
			request->subscriptionId = subscription->subscriptionId;

			UAε( UA_Client_MonitoredItems_createDataChanges_async(_client->UAPointer(), *request, contexts, dataChangeCallbacks.data(), deleteCallbacks.data(), createDataChangesCallback, this, &_requestId) );
			UA_CreateMonitoredItemsRequest_clear( &*request );
			//TRACET( MonitoringTag, "[{:x}.{:x}]DataSubscriptions - {}", Handle(), requestId, serialize(request.ToJson()) );
			_client->Process( _requestId, "MonitoredItems_createDataChanges" );//TODO handle BadSubscriptionIdInvalid
		}
		catch( UAException& e ){
			ResumeExp( move(e) );
		}
	}
	α DataChangeAwait::OnComplete( UA_CreateMonitoredItemsResponse* response )ι->void{
		_client->ClearRequest( _requestId );
		let sc = response->responseHeader.serviceResult;
		TRACE( "[{}.{}]CreateDataChangesCallback: {}", hex(_client->Handle()), hex(_requestId), UAException::Message(sc) );
		if( sc )
			ResumeExp( UAClientException{sc, _client->Handle(), _requestId} );//TODO clear monitored items
		else{
			_client->MonitoredNodes().OnCreateResponse( response, _monitoredRequestId );
			_h.resume();
		}
	}
	α DataChangeAwait::await_resume()ι->FromServer::SubscriptionAck{
		StatusCode sc{};
		if( up<IException> e = Promise() && Promise()->Exp() ? Promise()->MoveExp() : nullptr; e )
			sc = e->Code ? (StatusCode)e->Code : UA_STATUSCODE_BADINTERNALERROR;
		return FromServer::SubscriptionAck{ _client->MonitoredNodes().GetResult(_monitoredRequestId, sc) };
	}
}
