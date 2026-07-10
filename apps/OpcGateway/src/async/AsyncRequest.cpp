#include "AsyncRequest.h"
#include "../UAClient.h"
#include <jde/fwk/process/execution.h>
#define let const auto

namespace Jde::Opc::Gateway{
	namespace asio = boost::asio;
	Duration _pingInterval;//config, set once at startup - safe to share.
	Duration _ttl;//config, set once at startup - safe to share.

	AsyncRequest::AsyncRequest()ι:
		_strand{ Executor()->get_executor() }
	{}

	α AsyncRequest::CancelPing()ι->void{//strand-only
		if( _pingTimer )
			_pingTimer->Cancel();
	}
	α AsyncRequest::Ping( sp<UAClient> client )ι->DurationTimer::Task{//strand-confined; `client` keeps `this` (a UAClient member) alive across the await.
		ASSERT( !_pingTimer );
		if( _pingTimer )
			co_return;
		_pingTimer.emplace( _pingInterval, _strand, SRCE_CUR );
		DBGT( EOpcLogTags::ProcessingLoop, "Pinging '{}' in '{}'", client->Target(), Chrono::ToString(_pingInterval) );
		auto result = co_await *_pingTimer;//resumes on _strand
		_pingTimer.reset();
		if( result )
			client->Process( PingRequestId, "ping" );
		else
			CodeException resultEx{ result.error(), (ELogTags)EOpcLogTags::ProcessingLoop };
	}
	// 1 per UAClient. Runs entirely on _strand: started there by Process, and every co_await below resumes there
	// (executor-bound DurationTimer), so _requests/_client/_lastRequest/_pingTimer need no locks and
	// UA_Client_run_iterate never overlaps other UA_Client_* calls - submissions and sync services are dispatched
	// to the same strand via UAClient::PostUA.
	α AsyncRequest::ProcessingLoop()ι->DurationTimer::Task{
		ASSERT( _strand.running_in_this_thread() );
		sp<UAClient> client = _client;//keeps the owning UAClient (and thus `this`) alive across suspensions.
		function<string()> logPrefix = [client](){ return Ƒ("[{:x}]", client ? client->Handle() : 0); };
		DBG( "{}ProcessingLoop started", logPrefix() );
		CancelPing();
		StatusCode sc{};
		while( _running.test() ){
			let size = _requests.size();
			if( size ){
				if( *_requests.begin()==PingRequestId )
					_requests.erase( PingRequestId );
				else
					_lastRequest = Clock::now();
			}
			TRACE( "{}run_iterate: requestCount: {}", logPrefix(), size );
			if( sc = UA_Client_run_iterate(*client, 0); sc ){
				_running.clear();
				let level = _requests.size()>0 ? ELogLevel::Critical : ELogLevel::Debug;
				string requests;
				for_each( _requests, [&requests](auto r){requests += Ƒ("{:x}, ", r);} );
				LOG( level, _tags, "{}UA_Client_run_iterate returned ({:x}){}, requests: [{}]", logPrefix(), sc, UAException::Message(sc), requests );
				_requests.clear();
				break;
			}
			let newSize = _requests.size();
			if( !newSize ){
				TRACE( "{}requestCount: {}", logPrefix(), newSize );
				_running.clear();
				if( let now = Clock::now(); _lastRequest + _ttl < now ){
					DBG( "{}No requests for {}, shutting down client.", logPrefix(), Chrono::ToString(now-_lastRequest) );
					UAClient::ShutdownIdle( client );//per-client teardown: _lastRequest is per-client, so this client idling out must not tear down the others.
				}
				break;
			}
			let firstRequest = *_requests.begin();
			TRACE( "{}requestCount: {}, [0]={}", logPrefix(), newSize, hex(firstRequest) );
			if( size==newSize ){
				let sleep = size==1 && firstRequest==SubscriptionRequestId ? 500ms : 1ms; //UA_CreateSubscriptionRequest_default
				(void)co_await DurationTimer{ sleep, _strand, SRCE_CUR };
			}
		}
		if( !sc && !_stopped.test() && _pingInterval.count()>0 ){
			//post rather than call: a cancelled ping's resumption may still be queued on the strand (it resets _pingTimer);
			//strand FIFO guarantees it runs before this starter, so Ping always sees a settled _pingTimer.
			asio::post( _strand, [this, client]{
				if( !_stopped.test() && !_pingTimer )
					Ping( client );
			});
		}
		else
			DBG( "{}ProcessingLoop stopped", logPrefix() );
	}

	α AsyncRequest::Clear( RequestId requestId )ι->void{//strand-only (cross-thread callers go through UAClient::ClearRequest)
		ASSERT( _strand.running_in_this_thread() );
		TRACE( "[{}.{}]Clearing", hex(UAHandle()), hex(requestId) );
		if( !_requests.erase(requestId) && requestId!=ConnectRequestId )
			CRITICALT( ProcessingLoopTag, "[{}.{}]Could not find request handle.", hex(UAHandle()), hex(requestId) );
	}

	α AsyncRequest::Process( RequestId requestId, sv what )ι->void{//strand-only (cross-thread callers go through UAClient::Process)
		ASSERT( _strand.running_in_this_thread() );
		TRACE( "[{}.{}]Processing: {}", hex(UAHandle()), hex(requestId), what );
		if( _stopped.test() || !_client )
			return;
		_requests.emplace( requestId );
		if( !_running.test_and_set() )
			ProcessingLoop();
	}
	α AsyncRequest::Stop()ι->void{//strand-only (cross-thread callers go through UAClient::StopProcessing)
		ASSERT( _strand.running_in_this_thread() );
		DBG( "[{}]Stopping ProcessingLoop", UAHandle() );
		CancelPing();
		_stopped.test_and_set();
		_requests.clear();
		_running.clear();
		_client=nullptr;//breaks the UAClient<->AsyncRequest._client self-reference; the dispatching closure's keep-alive lets the UAClient be destroyed afterwards.
	}
	α AsyncRequest::UAHandle()ι->Handle{ return _client ? _client->Handle() : 0; }
}
