#include "AsyncRequest.h"
#include "../UAClient.h"
#include <jde/fwk/process/execution.h>
#include <stdexcept>
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
		optional<steady_clock::time_point> drainStart;//when our queue emptied with open62541's own read still outstanding - see below; steady: an NTP step must not stretch or truncate the drain window.
		let drainLimit = std::chrono::milliseconds{ UA_Client_getConfig(*client)->timeout } + 1s;//open62541 abandons a request at `timeout`, and its housekeeping only sweeps timeouts once a second; past that, pumping cannot complete the read.
		try{
			while( _running.test() ){
				let size = _requests.size();
				if( size ){
					drainStart.reset();//before the trace guard, so the iteration that picks up a mid-drain request keeps its queue snapshot; the drain window re-arms fresh if the queue empties again.
					if( *_requests.begin()==PingRequestId )
						_requests.erase( PingRequestId );
					else
						_lastRequest = Clock::now();
				}
				if( !drainStart )//tracing every drain poll buries the log.
					TRACE( "{}run_iterate: requestCount: {}", logPrefix(), size );
				if( sc = UA_Client_run_iterate(*client, 0); sc ){
					_running.clear();
					let level = _requests.size()>0 ? ELogLevel::Critical : ELogLevel::Debug;
					string requests;
					for_each( _requests, [&requests](auto r){requests += Ƒ("{:x}, ", r);} );
					LOG( level, _tags, "{}UA_Client_run_iterate returned ({}){}, requests: [{}]", logPrefix(), hex(sc), UAException::Message(sc), requests );
					_requests.clear();
					break;
				}
				let newSize = _requests.size();
				if( !newSize ){
					//open62541 issues requests of its own that never reach _requests - the server namespace-array read it fires
					//the moment the session activates is the one that bit us - and both their responses and their staleness
					//sweep are only serviced from run_iterate.  Leaving the loop as soon as our own queue drained left that
					//read unpumped until the ping woke us _pingInterval later, by which point open62541 had already given up
					//on it ("No result in the read namespace array response") and then discarded the reply that had been
					//sitting in the socket the whole time ("Request with unknown RequestId").  So while that read is
					//outstanding (_drainNeeded: flagged at activation by StateCallback, cleared by OnServiceBegin when the
					//reply is serviced) keep polling, bounded by drainLimit.
					if( _drainNeeded ){
						let now = steady_clock::now();
						if( !drainStart )
							drainStart = now;
						if( *drainStart+drainLimit > now ){
							//10ms, not the request path's 1ms: the drain only shepherds open62541's own read, which times out in
							//multiple seconds - and a request landing mid-drain waits at most one coarse step before the loop
							//re-enters the 1ms path (its submission already went out on the caller's strand post).
							if( let result = co_await DurationTimer{ 10ms, _strand }; !result ){
								CodeException{ result.error(), _tags };//an error-completing timer resumes instantly - continuing would busy-spin run_iterate for the rest of the window; log like the ping path and hand off to its timer.
								_running.clear();
								break;
							}
							continue;
						}
						WARN( "{}Drained {} without open62541's own request completing - handing the client back to the ping timer.", logPrefix(), Chrono::ToString(now-*drainStart) );
						_drainNeeded = false;
					}
					TRACE( "{}requestCount: {}", logPrefix(), newSize );
					_running.clear();
					if( let now = Clock::now(); _lastRequest + _ttl < now ){
						DBG( "{}No requests for {}, shutting down client.", logPrefix(), Chrono::ToString(now-_lastRequest) );
						_stopping = true;//teardown is committed: ShutdownIdle's delete-subscription pump restarts this loop (Subscriptions.cpp Process), and neither that instance's exit nor ours may arm a ping - it would race RemoveClient's Stop, and if the round trip outlives _pingInterval the fired ping restarts the loop on the half-torn-down client and the still-stale _lastRequest runs ShutdownIdle twice.
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
		}
		catch( Exception& e ){ //I don't see the use case here.
			e.SetLevel(ELogLevel::Critical);
			_running.clear();
		}
		if( !sc && !_stopped.test() && !_stopping && _pingInterval.count()>0 ){
			//post rather than call: a cancelled ping's resumption may still be queued on the strand (it resets _pingTimer);
			//strand FIFO guarantees it runs before this starter, so Ping always sees a settled _pingTimer.
			asio::post( _strand, [this, client]{
				if( !_stopped.test() && !_stopping && !_pingTimer )//re-check _stopping: a starter posted before the TTL branch set it can still run after.
					Ping( client );
			});
		}
		else if( sc && !_stopped.test() ){
			//run_iterate failed on a client nothing else is tearing down (a connecting client's failure StateCallback and
			//a request callback's Retry both Stop() inline before run_iterate returns, setting _stopped).  Skipping the
			//ping starter would leave the client in _clients with neither loop nor ping - a zombie every later request
			//trips over - so deregister it and let the next request build a fresh connection.  No MonitoredNodes
			//shutdown first: the transport is dead, so the delete-subscription round trip could never complete.
			INFO( "{}run_iterate failed - deregistering; the next request will reconnect.", logPrefix() );
			UAClient::RemoveClient( move(client) );
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

	α AsyncRequest::OnServiceBegin( RequestId requestId )ι->void{//strand-only (fires inside run_iterate)
		if( _drainNeeded && !_requests.contains(requestId) ){
			DBG( "[{}.{}]open62541's own request serviced - the post-activation drain can end.", hex(UAHandle()), hex(requestId) );
			_drainNeeded = false;
		}
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
