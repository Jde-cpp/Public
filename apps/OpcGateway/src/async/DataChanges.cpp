#include "DataChanges.h"
#include "../UAClient.h"
#include <jde/opc/uatypes/Value.h>
#include "../usings.h"

#define let const auto

namespace Jde::Opc::Gateway{
	static ELogTags _tags{ (ELogTags)(EOpcLogTags::Opc | EOpcLogTags::Monitoring) };
	α CreateDataChangesCallback( UA_Client* ua, void *userdata, RequestId requestId, UA_CreateMonitoredItemsResponse* response )ι->void{
		auto client = UAClient::TryFind(ua); if( !client ) return;
		auto h = client->ClearRequestH<DataChangeAwait::Handle>( requestId );  if( !h ){ CRITICAL("[{:x}.{:x}]Could not find handle.", (uint)ua, requestId ); return; }
		TRACE( "[{}.{}]CreateDataChangesCallback - {}", hex((uint)ua), hex(requestId), hex((Handle)userdata) );
		if( let sc = response->responseHeader.serviceResult; sc )
			h.promise().ResumeExp( UAClientException{sc, client->Handle(), requestId}, h );
		else{
			client->MonitoredNodes().OnCreateResponse( response, (Handle)userdata );
			h.resume();
		}
	}

	α DataChangesCallback( UA_Client* ua, SubscriptionId subId, void* /*subContext*/, MonitorId monId, void* /*monContext*/, UA_DataValue* uaValue )->void{
		auto pClient = UAClient::TryFind(ua); if(!pClient) return;
		Value value{ move(*uaValue) };
		let h = MonitorHandle{ subId, monId };
		TRACET( DataChangesTag, "[{:x}.{:x}] DataChangesCallback - {}", (uint)ua, (Handle)h, serialize(value.ToJson()) );
		if( !pClient->MonitoredNodes().SendDataChange(h, move(value)) )
			DBGT( DataChangesTag, "[{:x}.{:x}]Could not find node monitored item.", (uint)ua, (Handle)MonitorHandle{subId, monId} );
	}

	α DataChangesDeleteCallback( UA_Client* ua, SubscriptionId subId, void* /*_subContext_*/, MonitorId monId, void* /*_monContext_*/ )->void{
		TRACE( "[{:x}.{:x}]DataChangesDeleteCallback", (uint)ua, (Handle)MonitorHandle{subId, monId} );
	}

	α DataChangeAwait::Suspend()ι->void{
		_client->MonitoredNodes().Subscribe( move(_dataChange), move(_nodes), _h, _requestId );
	}
	α DataChangeAwait::await_resume()ι->FromServer::SubscriptionAck{
		StatusCode sc{};
		if( up<IException> e = Promise() && Promise()->Exp() ? Promise()->MoveExp() : nullptr; e )
			sc = e->Code ? (StatusCode)e->Code : UA_STATUSCODE_BADINTERNALERROR;
		return FromServer::SubscriptionAck{ _client->MonitoredNodes().GetResult(_requestId, sc) };
	}
}
