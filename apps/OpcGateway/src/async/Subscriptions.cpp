#include "Subscriptions.h"
#include <jde/fwk/process/execution.h>
#include "../UAClient.h"
#include "../types/UAClientException.h"
#include "MonitoredItems.h"

#define let const auto

namespace Jde::Opc::Gateway{
	static ELogTags _tags{ (ELogTags)(EOpcLogTags::Monitoring) };
	Ω statusChangeNotificationCallback( UA_Client* ua, UA_UInt32 subId, void* /*subContext*/, UA_StatusChangeNotification* /*notification*/ )ι->void{
		BREAK;
		TRACE( "[{:x}.{}]StatusChangeNotificationCallback", (uint)ua, subId );
	}

	Ω deleteSubscriptionCallback( UA_Client* ua, UA_UInt32 subId, void* /*subContext*/ )ι->void{
		if( auto client = UAClient::TryFind(ua); client ){
			INFO( "[{}.{}]DeleteSubscriptionCallback", hex(client->Handle()), subId );
			client->CreatedSubscriptionResponse = nullptr;
		}
	}

	α SubscribeAwait::await_ready()ι->bool{ return _client->CreatedSubscriptionResponse!=nullptr; }

	flat_map<sp<UAClient>,vector<SubscribeAwait::Handle>> _requests; mutex _requestsMutex;
	Ω createSubscriptionCallback( UA_Client*, void* userdata, RequestId, UA_CreateSubscriptionResponse* response )ι->void{
		auto await = (SubscribeAwait*)userdata;
		await->OnComplete( *response );
	}
	α SubscribeAwait::Suspend()ι->void{
		try{
			lg _{ _requestsMutex };
			auto& clientRequests = _requests[_client];
			clientRequests.push_back( _h );
			if( clientRequests.size()==1 ){
				UACε( UA_Client_Subscriptions_create_async(_client->UAPointer(), UA_CreateSubscriptionRequest_default(), nullptr, statusChangeNotificationCallback, deleteSubscriptionCallback, createSubscriptionCallback, this, &_requestId) );
				TRACE( "[{}.{}]CreateSubscription", hex(_client->Handle()), hex(_requestId) );
				_client->Process( _requestId, nullptr, "Subscriptions_create" );
			}
			else
				TRACE( "[{}.{}]CreateSubscription - queued", hex(_client->Handle()), hex(_requestId) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α SubscribeAwait::OnComplete( UA_CreateSubscriptionResponse& response )ι->void{
		_client->ClearRequest( _requestId );
		let sc = response.responseHeader.serviceResult;
		TRACE( "[{}.{}]createSubscriptionCallback - subscriptionId: {}, sc: {}", hex(_client->Handle()), hex(_requestId), response.subscriptionId, hex(sc) );
		lg _{ _requestsMutex };
		if( !sc )
			_client->CreatedSubscriptionResponse = ms<UA_CreateSubscriptionResponse>( move(response) );
		if( auto clientRequests = _requests.find(_client); clientRequests != _requests.end() ){
			for( auto&& h : clientRequests->second ){
				if( sc )
					Post( move(h), UAClientException{sc, _client->Handle()} );
				else
					Post( move(h) );
			}
			_requests.erase( clientRequests );
		}
	}

	UnsubscribeAwait::UnsubscribeAwait( flat_map<UA_UInt32,flat_set<MonitorId>>&& subscriptions, sp<UAClient> client, SL sl )ι:
		VoidAwait{sl},
		_client{ move(client) },
		_subscriptions{ move(subscriptions) }{
		_client->StopProcessDataSubscriptions();
	}
	α UnsubscribeAwait::RemoveMonitoredItems()ι->VoidAwait::Task{
		if( _subscriptions.size() )
			co_await DeleteMonitoredItemsAwait{ _subscriptions, _client };
		Unsubscribe();
	}
	α UnsubscribeAwait::Unsubscribe()ι->void{
		UA_DeleteSubscriptionsRequest request{
			.subscriptionIdsSize = _subscriptions.size(),
			.subscriptionIds = ( UA_UInt32* )UA_Array_new( _subscriptions.size(), &UA_TYPES[UA_TYPES_UINT32] )
		};
		uint i=0;
		for( let& [subscriptionId, _] : _subscriptions )
			request.subscriptionIds[i++] = subscriptionId;
		auto onComplete = []( UA_Client*, void* userdata, RequestId, UA_DeleteSubscriptionsResponse* response )ι->void {
			UnsubscribeAwait& await = *(UnsubscribeAwait*)userdata;
			await.OnComplete( *response );
		};
		UA_Client_Subscriptions_delete_async( _client->UAPointer(), request, onComplete, this, &_requestId );
		UA_DeleteSubscriptionsRequest_clear( &request );
		_client->Process( _requestId, nullptr, "Subscriptions_delete" );
		TRACE( "[{}.{}]Unsubscribe", hex(_client->Handle()), hex(_requestId) );
	}

	α UnsubscribeAwait::OnComplete( UA_DeleteSubscriptionsResponse& response )ι->void{
		_client->ClearRequest( _requestId );
		TRACE( "[{}.{}]UnsubscribeCallback count={}", hex(_client->Handle()), hex(_requestId), _subscriptions.size() );
		if( let sc = response.responseHeader.serviceResult; sc )
			WARN( "[{}.{}]Could not delete subscriptions:  {}.", hex(_client->Handle()), hex(_requestId), UAException::Message(sc) );
		for( auto sc : Iterable<UA_StatusCode>(response.results, response.resultsSize) ){
			if( sc )
				WARN( "[{}.{}]Could not delete subscription:  {}.", hex(_client->Handle()), hex(_requestId), UAException::Message(sc) );
		}
		Resume();
	}

}