#include "SetMonitoringMode.h"
#define let const auto

namespace Jde::Opc::Gateway{
	boost::concurrent_flat_map<sp<UAClient>,vector<SetMonitoringModeAwait::Handle>> _monitoringRequests;
	α SetMonitoringModeCallback( UA_Client* ua, void* /*userdata*/, RequestId requestId, UA_SetMonitoringModeResponse* response )ι->void{
		auto client = UAClient::TryFind(ua); if( !client ) return;
		client->ClearRequest( requestId );
		StatusCode sc{ response->responseHeader.serviceResult };
		for( uint i=0; i<response->resultsSize; ++i ){
			sc = response->results[i];
			if( !sc )
				break;
		}
		if( sc )
			SetMonitoringModeAwait::Resume( sc, move(client) );
		else{
			client->MonitoringModeResponse = ms<UA_SetMonitoringModeResponse>(move(*response));
			SetMonitoringModeAwait::Resume( move(client) );
		}
	}

	α SetMonitoringModeAwait::await_ready()ι->bool{ return _client->MonitoringModeResponse!=nullptr; }
	α SetMonitoringModeAwait::Suspend()ι->void{
		if( _monitoringRequests.try_emplace_or_visit(_client, vector<SetMonitoringModeAwait::Handle>{_h}, [ h=_h](auto x){x.second.push_back(h);}) )
			_client->SetMonitoringMode( _subscriptionId );
	}

	α SetMonitoringModeAwait::await_resume()ι->sp<UA_SetMonitoringModeResponse>{
		AwaitResume();
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

	α SetMonitoringModeAwait::Resume( sp<UAClient> _client )ι->void{
		Resume( _client, [_client](SetMonitoringModeAwait::Handle h){
			h.promise().Resume( sp<UA_SetMonitoringModeResponse>{_client->MonitoringModeResponse}, h );
		});
	}
	α SetMonitoringModeAwait::Resume( StatusCode sc, sp<UAClient>&& pClient )ι->void{
		Resume( move(pClient), [sc](SetMonitoringModeAwait::Handle&& h){
			h.promise().SetExp( UAException{sc} );
			h.resume();
		} );
	}
}