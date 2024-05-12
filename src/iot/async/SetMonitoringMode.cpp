#include <jde/iot/async/SetMonitoringMode.h>
#define var const auto

namespace Jde::Iot{
	boost::concurrent_flat_map<sp<UAClient>,vector<HCoroutine>> _monitoringRequests;
	α SetMonitoringModeCallback( UA_Client* ua, void* userdata, RequestId requestId, void* response )ι->void
	{
		var pResponse = static_cast<UA_SetMonitoringModeResponse*>( response );
		auto ppClient = Try<sp<UAClient>>( [ua](){return UAClient::Find(ua);} ); if( !ppClient ) return;
		(*ppClient)->ClearRequest<UARequest>( requestId );
		StatusCode sc{ pResponse->responseHeader.serviceResult };
		for( uint i=0; i<pResponse->resultsSize; ++i )
		{
			sc = pResponse->results[i];
			if( !sc )
				break;
		}
		if( sc )
			SetMonitoringModeAwait::Resume( sc, move(*ppClient) );
		else{
			(*ppClient)->MonitoringModeResponse = ms<UA_SetMonitoringModeResponse>(move(*pResponse));
			SetMonitoringModeAwait::Resume( move(*ppClient) );
		}
	}

	α SetMonitoringModeAwait::await_ready()ι->bool{ return _client->MonitoringModeResponse!=nullptr; }
	α SetMonitoringModeAwait::await_suspend( HCoroutine h )ι->void{
		IAwait::await_suspend( h );
		if( _monitoringRequests.try_emplace_or_visit(_client, vector<HCoroutine>{h}, [ h2=move(h)](auto x){x.second.push_back(move(h2));}) )
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
		if( !_monitoringRequests.cvisit(pClient, [resume](var& x){
			for( auto h : x.second )
				resume( move(h) );
			}) )
			CRITICALT( AppTag(), "Could not find client ({:x}) for SetMonitoringModeAwait", (uint)pClient->UAPointer() );

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