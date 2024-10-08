﻿#include <jde/iot/async/CreateSubscriptions.h>
#define var const auto

namespace Jde::Iot{
	static ELogTags _tag{ (ELogTags)(EIotLogTags::Iot | EIotLogTags::Monitoring) };
	boost::concurrent_flat_map<sp<UAClient>,vector<HCoroutine>> _subscriptionRequests;
	α CreateSubscriptionCallback(UA_Client* ua, void* /*userdata*/, RequestId requestId, void *response)ι->void{
		auto pResponse = static_cast<UA_CreateSubscriptionResponse*>( response );
		auto pClient = UAClient::TryFind(ua); if( !pClient ) return;
		pClient->ClearRequest<UARequest>( requestId );
		if( var sc = pResponse->responseHeader.serviceResult; sc )
			CreateSubscriptionAwait::Resume( UAException{sc}, move(pClient) );
		else{
			Trace( _tag, "[{:x}.{}]CreateSubscriptionCallback - subscriptionId={}", (uint)ua, requestId, pResponse->subscriptionId );
			pClient->CreatedSubscriptionResponse = ms<UA_CreateSubscriptionResponse>(move(*pResponse));
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
		if( _subscriptionRequests.try_emplace_or_visit(_client, vector<HCoroutine>{_h}, [h=_h](auto x){x.second.push_back(h);}) )
			_client->CreateSubscriptions();
	}

	α CreateSubscriptionAwait::await_resume()ι->AwaitResult{
		ASSERT( _client->CreatedSubscriptionResponse || (_pPromise && _pPromise->HasError()) );
		return _pPromise
			? _pPromise->MoveResult()
			: AwaitResult{ _client->CreatedSubscriptionResponse };
	}
	α CreateSubscriptionAwait::Resume( sp<UAClient> pClient, function<void(HCoroutine&&)> resume )ι->void{
		ASSERT( pClient );
		if( !_subscriptionRequests.cvisit(pClient, [resume](var& x){for( auto h : x.second ) resume( move(h) );}) )
			Critical( _tag, "Could not find client ({:x}) for CreateSubscriptionAwait", (uint)pClient->UAPointer() );

		_subscriptionRequests.erase( pClient );
	}

	α CreateSubscriptionAwait::Resume( sp<UAClient> pClient )ι->void{
		Resume( pClient, [pClient](HCoroutine&& h)
		{
			h.promise().SetResult( sp<UA_CreateSubscriptionResponse>{pClient->CreatedSubscriptionResponse} );
			Coroutine::CoroutinePool::Resume( move(h) ); //Cannot run EventLoop from the run method itself
		});
	}
	α CreateSubscriptionAwait::Resume( UAException&& e, sp<UAClient>&& pClient )ι->void{
		Resume( move(pClient), [e2=move(e)](HCoroutine&& h)mutable{Jde::Resume(move(e2), move(h));} );
	}
}