#include "SetMonitoringMode.h"
#define let const auto

namespace Jde::Opc::Gateway{
	boost::concurrent_flat_map<sp<UAClient>,vector<SetMonitoringModeAwait::Handle>> _monitoringRequests;
	Ω callback( UA_Client* /*ua*/, void* userdata, RequestId /*requestId*/, UA_SetMonitoringModeResponse* response )ι->void{
		SetMonitoringModeAwait& await = *(SetMonitoringModeAwait*)userdata;
		await.OnComplete( *response );
	}

	α SetMonitoringModeAwait::await_ready()ι->bool{ return _client->MonitoringModeResponse!=nullptr; }
	α SetMonitoringModeAwait::Suspend()ι->void{
		if( !_monitoringRequests.try_emplace_or_visit(_client, vector<SetMonitoringModeAwait::Handle>{_h}, [ h=_h](auto x){x.second.push_back(h);}) )
			return;
		UA_SetMonitoringModeRequest request{};
		request.subscriptionId = _subscriptionId;
		try{
			UAε( UA_Client_MonitoredItems_setMonitoringMode_async(_client->UAPointer(), move(request), callback, this, &_requestId) );
			_client->Process( _requestId, "setMonitoringMode" );
		}
		catch( UAException& e ){
			Resume( (StatusCode)e.Code, _client );
		}
	}
	α SetMonitoringModeAwait::OnComplete( UA_SetMonitoringModeResponse& response )ι->void{
		_client->ClearRequest( _requestId );
		StatusCode sc{ response.responseHeader.serviceResult };
		for( uint i=0; i<response.resultsSize; ++i ){
			if( sc = response.results[i]; !sc )
				break;
		}
		if( sc )
			SetMonitoringModeAwait::Resume( sc, _client );
		else{
			_client->MonitoringModeResponse = ms<UA_SetMonitoringModeResponse>( move(response) );
			SetMonitoringModeAwait::Resume( _client );
		}
	}

	α SetMonitoringModeAwait::await_resume()ι->sp<UA_SetMonitoringModeResponse>{
		CheckException();
		ASSERT( _client->MonitoringModeResponse );
		return _client->MonitoringModeResponse;
	}
	α SetMonitoringModeAwait::Resume( sp<UAClient> _client, function<void(SetMonitoringModeAwait::Handle&&)> resume )ι->void{
		ASSERT( _client );
		if( !_monitoringRequests.cvisit(_client, [resume](let& x){
			for( auto h : x.second )
				resume( move(h) );
			}) ){
			CRITICALT( ELogTags::App, "Could not find client ({:x}) for SetMonitoringModeAwait", (uint)_client->UAPointer() );
		}
		_monitoringRequests.erase( _client );
	}

	α SetMonitoringModeAwait::Resume( sp<UAClient> client )ι->void{
		Resume( client, [client](SetMonitoringModeAwait::Handle h){
			h.promise().Resume( sp<UA_SetMonitoringModeResponse>{client->MonitoringModeResponse}, h );
		});
	}
	α SetMonitoringModeAwait::Resume( StatusCode sc, sp<UAClient> client )ι->void{
		Resume( move(client), [sc](SetMonitoringModeAwait::Handle&& h){
			h.promise().SetExp( UAException{sc} );
			h.resume();
		} );
	}
}