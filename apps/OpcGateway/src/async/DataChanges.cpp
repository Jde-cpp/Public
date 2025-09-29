#include "DataChanges.h"
#include "../UAClient.h"
#include <jde/opc/uatypes/Value.h>

#define let const auto

namespace Jde::Opc::Gateway{
	static ELogTags _tags{ (ELogTags)(EOpcLogTags::Opc | EOpcLogTags::Monitoring) };
	α CreateDataChangesCallback( UA_Client* ua, void *userdata, RequestId requestId, UA_CreateMonitoredItemsResponse* response )ι->void{
		auto client = UAClient::TryFind(ua); if( !client ) return;
		auto h = client->ClearRequestH<DataChangeAwait::Handle>( requestId );  if( !h ){ CRITICAL("[{:x}.{:x}]Could not find handle.", (uint)ua, requestId ); return; }
		TRACE( "[{:x}.{:x}]CreateDataChangesCallback - {:x}", (uint)ua, requestId, (Handle)userdata );
		if( let sc = response->responseHeader.serviceResult; sc )
			h.promise().ResumeExp( UAClientException{sc, client->Handle(), requestId}, h );
		else{
			client->MonitoredNodes.OnCreateResponse( response, (Handle)userdata );
			h.resume();
		}
	}
	α MonitoredItemsDeleteCallback( UA_Client* ua, void* /*_userdata_*/, RequestId requestId, UA_DeleteMonitoredItemsResponse* response )ι->void{
		auto pClient = UAClient::TryFind(ua); if( !pClient ) return;
		pClient->ClearRequest( requestId );
		TRACE( "[{:x}.{:x}]MonitoredItemsDeleteCallback", (uint)ua, requestId );
		if( let sc = response->responseHeader.serviceResult; sc )
			WARN( "[{:x}.{:x}]Could not delete monitored items:  {}.", (uint)ua, requestId, UAException::Message(sc) );
    for( auto sc : Iterable<UA_StatusCode>(response->results, response->resultsSize) ){
			if( sc )
				WARN( "[{:x}.{:x}]Could not delete monitored item:  {}.", (uint)ua, requestId, UAException::Message(sc) );
		}
	}

	α DataChangesCallback( UA_Client* ua, SubscriptionId subId, void* /*subContext*/, MonitorId monId, void* /*monContext*/, UA_DataValue* uaValue )->void{
		auto pClient = UAClient::TryFind(ua); if(!pClient) return;
		Value value{ move(*uaValue) };
		let h = MonitorHandle{ subId, monId };
		TRACET( DataChangesTag, "[{:x}.{:x}] DataChangesCallback - {}", (uint)ua, (Handle)h, serialize(value.ToJson()) );
		if( !pClient->MonitoredNodes.SendDataChange(h, move(value)) )
			DBGT( DataChangesTag, "[{:x}.{:x}]Could not find node monitored item.", (uint)ua, (Handle)MonitorHandle{subId, monId} );
	}

	α DataChangesDeleteCallback( UA_Client* ua, SubscriptionId subId, void* /*_subContext_*/, MonitorId monId, void* /*_monContext_*/ )->void{
		TRACE( "[{:x}.{:x}]DataChangesDeleteCallback", (uint)ua, (Handle)MonitorHandle{subId, monId} );
	}

	α DataChangeAwait::Suspend()ι->void{
		_client->MonitoredNodes.Subscribe( move(_dataChange), move(_nodes), _h, _requestId );
	}
	α DataChangeAwait::await_resume()ι->FromServer::SubscriptionAck{
		StatusCode sc{};
		if( up<IException> e = Promise() && Promise()->Exp() ? Promise()->MoveExp() : nullptr; e )
			sc = e->Code ? (StatusCode)e->Code : UA_STATUSCODE_BADINTERNALERROR;
		return FromServer::SubscriptionAck{ _client->MonitoredNodes.GetResult(_requestId, sc) };
	}
/*
	CreateDataChangesResult::CreateDataChangesResult( CreateMonitoredItemsRequest&& request, CreateMonitoredItemsResponsePtr&& result )ι:
		Result{move(result)}{
		ASSERT(request.itemsToCreateSize==Result->resultsSize);
		for( uint i=0; i<request.itemsToCreateSize; ++i )
			Nodes.emplace_back( request.itemsToCreate[i].itemToMonitor.nodeId );
	}
	α	CreateDataChangesResult::ForEach( std::function<void(const NodeId&, const UA_MonitoredItemCreateResult&)>&& f )ι->void{
		for( uint i=0; i<std::min(Nodes.size(), Result->resultsSize); ++i )
			f( Nodes[i], Result->results[i] );
	}
	α	CreateDataChangesResult::Append( flat_map<NodeId, MonitoredItemCreateResult>&& existingNodes )ι->void{
		CreateMonitoredItemsResponsePtr response{ UA_CreateMonitoredItemsResponse_new() };
		uint subscribedSize = Result ? Result->resultsSize : 0;
		response->resultsSize = subscribedSize+existingNodes.size();
		response->results = (UA_MonitoredItemCreateResult*) UA_Array_new(response->resultsSize, &UA_TYPES[UA_TYPES_MONITOREDITEMCREATERESULT]);
		uint i=0;
		for( ;i<subscribedSize; ++i )
			response->results[i] = Result->results[i];
		for( auto&& [nodeId, result] : existingNodes ){
			response->results[i++] = move( result );
			Nodes.push_back( nodeId );
		}
		Result = move( response );
	}
	*/
}
