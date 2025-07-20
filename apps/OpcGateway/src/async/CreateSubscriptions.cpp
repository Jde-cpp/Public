#include "CreateSubscriptions.h"
#include "../UAClient.h"
#define let const auto

namespace Jde::Opc::Gateway{
	static ELogTags _tag{ (ELogTags)(EOpcLogTags::Opc | EOpcLogTags::Monitoring) };
	boost::concurrent_flat_map<sp<UAClient>,vector<CreateSubscriptionAwait::Handle>> _subscriptionRequests;
	α CreateSubscriptionCallback(UA_Client* ua, void* /*userdata*/, RequestId requestId, UA_CreateSubscriptionResponse* response)ι->void{
		auto pClient = UAClient::TryFind(ua); if( !pClient ) return;
		pClient->ClearRequest( requestId );
		if( let sc = response->responseHeader.serviceResult; sc )
			CreateSubscriptionAwait::Resume( UAException{sc}, move(pClient) );
		else{
			Trace( _tag, "[{:x}.{}]CreateSubscriptionCallback - subscriptionId={}", (uint)ua, requestId, response->subscriptionId );
			pClient->CreatedSubscriptionResponse = ms<UA_CreateSubscriptionResponse>(move(*response));
			CreateSubscriptionAwait::Resume( move(pClient) );
		}
	}

	α StatusChangeNotificationCallback(UA_Client* ua, UA_UInt32 subId, void* /*subContext*/, UA_StatusChangeNotification* /*notification*/)ι->void{
		BREAK;
		Trace( _tag, "[{:x}.{}]StatusChangeNotificationCallback", (uint)ua, subId );
	}

	α DeleteSubscriptionCallback( UA_Client* ua, UA_UInt32 subId, void* /*subContext*/ )ι->void{
		Trace( _tag, "[{:x}.{}]DeleteSubscriptionCallback", (uint)ua, subId );
		auto pClient = UAClient::TryFind(ua);
		if( pClient )
			pClient->CreatedSubscriptionResponse = nullptr;
	}

	α CreateSubscriptionAwait::await_ready()ι->bool{ return _client->CreatedSubscriptionResponse!=nullptr; }
	α CreateSubscriptionAwait::Suspend()ι->void{
		if( _subscriptionRequests.try_emplace_or_visit(_client, vector<CreateSubscriptionAwait::Handle>{_h}, [h=_h](auto x){x.second.push_back(h);}) )
			_client->CreateSubscriptions();
	}

	α CreateSubscriptionAwait::await_resume()ε->sp<UA_CreateSubscriptionResponse>{
		ASSERT( _client->CreatedSubscriptionResponse || (Promise() && Promise()->Exp()) );
		return Promise()
			? base::await_resume()
			: _client->CreatedSubscriptionResponse;
	}
	α CreateSubscriptionAwait::Resume( sp<UAClient> pClient, function<void(CreateSubscriptionAwait::Handle)> resume )ι->void{
		ASSERT( pClient );
		if( !_subscriptionRequests.cvisit(pClient, [resume](let& x){for( auto h : x.second ) resume( move(h) );}) )
			Critical( _tag, "Could not find client ({:x}) for CreateSubscriptionAwait", (uint)pClient->UAPointer() );

		_subscriptionRequests.erase( pClient );
	}

	α CreateSubscriptionAwait::Resume( sp<UAClient> pClient )ι->void{
		Resume( pClient, [pClient](CreateSubscriptionAwait::Handle h)
		{
			h.promise().SetValue( sp<UA_CreateSubscriptionResponse>{pClient->CreatedSubscriptionResponse} );
			Coroutine::CoroutinePool::Resume( move(h) ); //Cannot run EventLoop from the run method itself
		});
	}
	α CreateSubscriptionAwait::Resume( UAException&& e, sp<UAClient>&& pClient )ι->void{
		Resume( move(pClient), [e2=move(e)](CreateSubscriptionAwait::Handle h)mutable{
			h.promise().ResumeExp( move(e2), h );
		});
	}
}