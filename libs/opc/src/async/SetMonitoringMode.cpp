#include <jde/opc/async/SetMonitoringMode.h>
#define let const auto

namespace Jde::Opc{
	boost::concurrent_flat_map<sp<UAClient>,vector<HCoroutine>> _monitoringRequests;
	α SetMonitoringModeCallback( UA_Client* ua, void* /*userdata*/, RequestId requestId, UA_SetMonitoringModeResponse* response )ι->void
	{
		auto ppClient = Try<sp<UAClient>>( [ua](){return UAClient::Find(ua);} ); if( !ppClient ) return;
		(*ppClient)->ClearRequest<UARequest>( requestId );
		StatusCode sc{ response->responseHeader.serviceResult };
		for( uint i=0; i<response->resultsSize; ++i )
		{
			sc = response->results[i];
			if( !sc )
				break;
		}
		if( sc )
			SetMonitoringModeAwait::Resume( sc, move(*ppClient) );
		else{
			(*ppClient)->MonitoringModeResponse = ms<UA_SetMonitoringModeResponse>(move(*response));
			SetMonitoringModeAwait::Resume( move(*ppClient) );
		}
	}

	α SetMonitoringModeAwait::await_ready()ι->bool{ return _client->MonitoringModeResponse!=nullptr; }
	α SetMonitoringModeAwait::Suspend()ι->void{
		if( _monitoringRequests.try_emplace_or_visit(_client, vector<HCoroutine>{_h}, [ h=_h](auto x){x.second.push_back(h);}) )
			_client->SetMonitoringMode( _subscriptionId );
	}

	α SetMonitoringModeAwait::await_resume()ι->AwaitResult{
		ASSERT( _client->MonitoringModeResponse || (_pPromise && _pPromise->HasError()) );
		AwaitResult y;
		if( _pPromise )
			y = _pPromise->MoveResult();
		else
			y.Set( _client->MonitoringModeResponse );

		return y;
	}
	α SetMonitoringModeAwait::Resume( sp<UAClient> pClient, function<void(HCoroutine&&)> resume )ι->void{
		ASSERT( pClient );
		if( !_monitoringRequests.cvisit(pClient, [resume](let& x){
			for( auto h : x.second )
				resume( move(h) );
			}) )
			Critical( ELogTags::App, "Could not find client ({:x}) for SetMonitoringModeAwait", (uint)pClient->UAPointer() );

		_monitoringRequests.erase( pClient );
	}

	α SetMonitoringModeAwait::Resume( sp<UAClient> pClient )ι->void{
		Resume( pClient, [pClient](HCoroutine&& h){Jde::Resume((sp<UA_SetMonitoringModeResponse>)pClient->MonitoringModeResponse, move(h));} );
	}
	α SetMonitoringModeAwait::Resume( StatusCode sc, sp<UAClient>&& pClient )ι->void{
		Resume( move(pClient), [sc](HCoroutine&& h)
		{
			Jde::Resume( UAException{sc}, move(h) );
		} );
	}
}