#include "CreateSubscriptions.h"
#include <jde/fwk/process/execution.h>
#include "../UAClient.h"
#define let const auto

namespace Jde::Opc::Gateway{
	static ELogTags _tags{ (ELogTags)(EOpcLogTags::Monitoring) };
	flat_map<sp<UAClient>,vector<CreateSubscriptionAwait::Handle>> _requests; mutex _requestsMutex;
	Ω createSubscriptionCallback( UA_Client* ua, void* /*userdata*/, RequestId requestId, UA_CreateSubscriptionResponse* response )ι->void{
		auto client = UAClient::TryFind( ua );
		if( !client ){
			 CRITICAL( "[{}]Could not find client.", hex((uint)ua) );
			 return;
		}
		client->ClearRequest( requestId );
		let sc = response->responseHeader.serviceResult;
		TRACE( "[{}.{}]CreateSubscriptionCallback - subscriptionId={}, sc={}", client->Handle(), requestId, response->subscriptionId, sc );
		lg _{ _requestsMutex };
		if( !sc )
			client->CreatedSubscriptionResponse = ms<UA_CreateSubscriptionResponse>( move(*response) );
		if( auto clientRequests = _requests.find(client); clientRequests != _requests.end() ){
			for( auto&& h : clientRequests->second ){
				if( sc )
					Post( move(h), UAClientException{sc, client->Handle()} );
				else
					Post( move(h) );
			}
			_requests.erase( clientRequests );
		}
	}

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

	α CreateSubscriptionAwait::await_ready()ι->bool{ return _client->CreatedSubscriptionResponse!=nullptr; }
	α CreateSubscriptionAwait::Suspend()ι->void{
		try{
			lg _{ _requestsMutex };
			auto& clientRequests = _requests[_client];
			clientRequests.push_back( _h );
			if( clientRequests.size()==1 ){
				UAε( UA_Client_Subscriptions_create_async(_client->UAPointer(), UA_CreateSubscriptionRequest_default(), nullptr, statusChangeNotificationCallback, deleteSubscriptionCallback, createSubscriptionCallback, this, &_requestId) );
				TRACE( "[{}.{}]CreateSubscription", hex(_client->Handle()), hex(_requestId) );
				_client->Process( _requestId, nullptr, "Subscriptions_create" );
			}
			else
				TRACE( "[{}.{}]CreateSubscription - queued", hex(_client->Handle()), hex(_requestId) );
		}
		catch( UAException& e ){
			ResumeExp( move(e) );
		}
	}
}