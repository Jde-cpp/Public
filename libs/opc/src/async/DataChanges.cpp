#include <jde/opc/async/DataChanges.h>

#define let const auto

namespace Jde::Opc{
	static ELogTags _tag{ (ELogTags)(EOpcLogTags::Opc | EOpcLogTags::Monitoring) };
	α CreateDataChangesCallback( UA_Client* ua, void *userdata, RequestId requestId, void *response )ι->void{
		auto pClient = UAClient::TryFind(ua); if( !pClient ) return;
		auto pResponse = static_cast<UA_CreateMonitoredItemsResponse*>( response );
		auto pRequest = pClient->ClearRequest<UARequest>( requestId );  if( !pRequest ){ Critical(_tag, "[{:x}.{:x}]Could not find handle.", (uint)ua, requestId ); return; }
		Trace( _tag, "[{:x}.{:x}]CreateDataChangesCallback - {:x}", (uint)ua, requestId, (Handle)userdata );
		if( let sc = pResponse->responseHeader.serviceResult; sc )
			Resume( UAException{sc}, move(pRequest->CoHandle) );
		else{
			pClient->MonitoredNodes.OnCreateResponse( pResponse, (Handle)userdata );
			pRequest->CoHandle.resume();
		}
	}
	α MonitoredItemsDeleteCallback( UA_Client* ua, void* /*_userdata_*/, RequestId requestId, void* response )ι->void{
		auto pClient = UAClient::TryFind(ua); if( !pClient ) return;
		auto pResponse = static_cast<UA_DeleteMonitoredItemsResponse*>( response );
		pClient->ClearRequest<UARequest>( requestId );
		Trace( _tag, "[{:x}.{:x}]MonitoredItemsDeleteCallback", (uint)ua, requestId );
		if( let sc = pResponse->responseHeader.serviceResult; sc )
			Warning( _tag, "[{:x}.{:x}]Could not delete monitored items:  {}.", (uint)ua, requestId, UAException::Message(sc) );
    for( auto sc : Iterable<UA_StatusCode>(pResponse->results, pResponse->resultsSize) ){
			if( sc )
				Warning( _tag, "[{:x}.{:x}]Could not delete monitored item:  {}.", (uint)ua, requestId, UAException::Message(sc) );
		}
	}

	α DataChangesCallback( UA_Client* ua, SubscriptionId subId, void* /*subContext*/, MonitorId monId, void* /*monContext*/, UA_DataValue* uaValue )->void{
		auto pClient = UAClient::TryFind(ua); if(!pClient) return;
		Value value{ move(*uaValue) };
		let h = MonitorHandle{ subId, monId };
		Trace( DataChangesTag, "[{:x}.{:x}] DataChangesCallback - {}", (uint)ua, (Handle)h, serialize(value.ToJson()) );
		if( !pClient->MonitoredNodes.SendDataChange(h, move(value)) )
			Debug( DataChangesTag, "[{:x}.{:x}]Could not find node monitored item.", (uint)ua, (Handle)MonitorHandle{subId, monId} );
	}

	α DataChangesDeleteCallback( UA_Client* ua, SubscriptionId subId, void* /*_subContext_*/, MonitorId monId, void* /*_monContext_*/ )->void{
		Trace( _tag, "[{:x}.{:x}]DataChangesDeleteCallback", (uint)ua, (Handle)MonitorHandle{subId, monId} );
	}

	α DatachangeAwait::Suspend()ι->void{
		_client->MonitoredNodes.Subscribe( move(_dataChange), move(_nodes), _h, _requestId );
	}
	α DatachangeAwait::await_resume()ι->AwaitResult{
		StatusCode sc{};
		if( _pPromise && _pPromise->HasError() ){
			up<IException> e = _pPromise->MoveResult().Error();
			sc = e->Code ? (StatusCode)e->Code : UA_STATUSCODE_BADINTERNALERROR;
		}
		return AwaitResult{ mu<FromServer::SubscriptionAck>(_client->MonitoredNodes.GetResult(_requestId, sc)) };
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
