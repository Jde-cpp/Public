#include "AsyncRequest.h"
#include "../UAClient.h"
#include <jde/fwk/process/execution.h>
#define let const auto

namespace Jde::Opc::Gateway{
	Duration _pingInterval;//config, set once at startup - safe to share.
	Duration _ttl;//config, set once at startup - safe to share.
	//TODO! _lastRequest and _pingTimer are per-client state but live at file scope, shared across every UAClient's ProcessingLoop. With >=2 clients: _lastRequest is written under different per-client _requestMutexes (data race; one client's traffic suppresses another's TTL shutdown), and ping()'s ASSERT(!_pingTimer) fires / both coroutines co_await the same DurationTimer and the first reset()s it under the other (UB). Move both into AsyncRequest. Deferred to be fixed together with the #3 threading redesign.
	TimePoint _lastRequest;
	optional<DurationTimer> _pingTimer; mutex _pingMutex;

	Ω cancelPing()ι->void{
		lg _{ _pingMutex };
		if( _pingTimer )
			_pingTimer->Cancel();
	}
	Ω ping( sp<UAClient> client )ι->DurationTimer::Task{
		{
			lg _{ _pingMutex };
			ASSERT( !_pingTimer );
			if( !_pingTimer ){
				_pingTimer.emplace( _pingInterval, SRCE_CUR );
				DBGT( EOpcLogTags::ProcessingLoop, "Pinging '{}' in '{}'", client->Target(), Chrono::ToString(_pingInterval) );
			}
		}
		auto result = co_await *_pingTimer;
		{
			lg _{ _pingMutex };
			_pingTimer.reset();
		}
		if( result )
			client->Process( PingRequestId, "ping" );
		else
			CodeException resultEx{ result.error(), (ELogTags)EOpcLogTags::ProcessingLoop };
	}
	// 1 per UAClient
	α AsyncRequest::ProcessingLoop()ι->DurationTimer::Task{
		sp<UAClient> client;
		{ ul _{ _requestMutex }; client = _client; }//Stop() writes _client under the same mutex; read it once here to avoid the shared_ptr data race.
		function<string()> logPrefix = [client](){ return Ƒ("[{:x}]", client ? client->Handle() : 0); };
		DBG( "{}ProcessingLoop started", logPrefix() );
		cancelPing();
		StatusCode sc{};
		while( _running.test() ){
			uint size;
			{
				ul _{ _requestMutex };
				if( size = _requests.size(); size ){
					if( *_requests.begin()==PingRequestId )
						_requests.erase( PingRequestId );
					else
						_lastRequest = Clock::now();
				}
			}
			TRACE( "{}run_iterate: requestCount: {}", logPrefix(), size );
			//TODO! open62541 UA_Client is not thread-safe, but the fwk executor is a multi-threaded io_context pool.
			// run_iterate() runs here on one pool thread while other pool threads submit async requests (ReadAwait,
			// ReadValueAwait, WriteAwait, DataChanges, Subscriptions, ...) and call synchronous services
			// (translateBrowsePathsToNodeIds, getRemoteDataTypes, getConnectionAttributeCopy) against the same client.
			// Fix requires a design decision: route ALL UA_Client_* calls through this loop (queue of closures), or
			// guard run_iterate + every UA_Client_* call with a per-client mutex. Deferred (review finding #3).
			if( sc = UA_Client_run_iterate(*client, 0); sc ){
				_running.clear();
				ul _{ _requestMutex };
				let level = _requests.size()>0 ? ELogLevel::Critical : ELogLevel::Debug;
				string requests;
				for_each( _requests, [&requests](auto r){requests += Ƒ("{:x}, ", r);} );
				LOG( level, _tags, "{}UA_Client_run_iterate returned ({:x}){}, requests: [{}]", logPrefix(), sc, UAException::Message(sc), requests );
				_requests.clear();
				break;
			}
			uint newSize;
			RequestId firstRequest{};
			{
				_requestMutex.lock();
				if( newSize=_requests.size(); !newSize ){
					TRACE( "{}requestCount: {}", logPrefix(), newSize );
					_running.clear();
					_requestMutex.unlock();
					if( let now = Clock::now(); _lastRequest + _ttl < now ){
						DBG( "{}No requests for {}, shutting down client.", logPrefix(), Chrono::ToString(now-_lastRequest) );
						client->Shutdown();
					}
					break;
				}
				firstRequest = *_requests.begin();
				TRACE( "{}requestCount: {}, [0]={}", logPrefix(), newSize, hex(firstRequest) );
				_requestMutex.unlock();
			}
			if( size==newSize ){
				let sleep = size==1 && firstRequest==SubscriptionRequestId ? 500ms : 1ms; //UA_CreateSubscriptionRequest_default
				(void)co_await DurationTimer{ sleep };
			}
		}
		if( !sc && !_stopped.test() && _pingInterval.count()>0 )
			ping( client );
		else
			DBG( "{}ProcessingLoop stopped", logPrefix() );
	}

	α AsyncRequest::Clear( RequestId requestId )ι->void{
		TRACE( "[{}.{}]Clearing", hex(UAHandle()), hex(requestId) );
		ul _{_requestMutex};
		if( !_requests.erase(requestId) && requestId!=ConnectRequestId )
			CRITICALT( ProcessingLoopTag, "[{}.{}]Could not find request handle.", hex(UAHandle()), hex(requestId) );
	}

	α AsyncRequest::Process( RequestId requestId, sv what )ι->void{
		TRACE( "[{}.{}]Processing: {}", hex(UAHandle()), hex(requestId), what );
		if( _stopped.test() )
			return;
		sp<UAClient> self;
		{
			ul _{_requestMutex};
			_requests.emplace( requestId );
			self = _client;//keep the owning UAClient (and thus `this`) alive across the Post; Stop() nulls _client under this mutex.
		}
		if( self && !_running.test_and_set() )
			Post( [self,this]{ ProcessingLoop(); } );
	}
	α AsyncRequest::Stop()ι->void{
		DBG( "[{}]Stopping ProcessingLoop", UAHandle() );
		ul _{ _requestMutex };
		cancelPing();
		_stopped.test_and_set();
		_requests.clear();
		_running.clear();
		_client=nullptr;
	}
	α AsyncRequest::UAHandle()ι->Handle{ return _client ? _client->Handle() : 0; }
}