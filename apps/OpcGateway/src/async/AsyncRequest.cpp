#include "AsyncRequest.h"
#include "../UAClient.h"
#include <jde/fwk/process/thread.h>
#define let const auto

namespace Jde::Opc::Gateway{
	Duration _pingInterval;
	Duration _ttl;
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
			_pingTimer.emplace( _pingInterval, SRCE_CUR );
			DBGT( EOpcLogTags::ProcessingLoop, "Pinging '{}' in '{}'", client->Target(), Chrono::ToString(_pingInterval) );
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
		function<string()> logPrefix = [this](){ return Ƒ("[{:x}]", _client ? _client->Handle() : 0); };
		DBG( "{}ProcessingLoop started", logPrefix() );
		cancelPing();
		StatusCode sc{};
		while( _running.test() ){
			auto client = _client;
			uint size;
			{
				sl _{ _requestMutex };
				if( size = _requests.size(); size ){
					if( _requests.begin()->first==PingRequestId )
						_requests.erase( PingRequestId );
					else
						_lastRequest = Clock::now();
				}
			}
			TRACE( "{}run_iterate: requestCount: {}", logPrefix(), size );
			if( sc = UA_Client_run_iterate(*client, 0); sc ){
				ERR( "{}UA_Client_run_iterate returned ({:x}){}", logPrefix(), sc, UAException::Message(sc) );
				_running.clear();
				ul _{ _requestMutex };
				_requests.clear();
				break;
			}
			uint newSize;
			{
				_requestMutex.lock();
				if( newSize=_requests.size(); !newSize ){
					TRACE( "{}requestCount: {}", logPrefix(), newSize );
					_running.clear();
					_requestMutex.unlock();
					if( let now = Clock::now(); _lastRequest + _ttl < now ){
						DBG( "{}No requests for {}, shutting down client.", logPrefix(), Chrono::ToString(now-_lastRequest) );
						client->Shutdown( false );
					}
					break;
				}
				TRACE( "{}requestCount: {}, [0]={}", logPrefix(), newSize, hex(_requests.begin()->first) );
				_requestMutex.unlock();
			}
			if( size==newSize ){
				let sleep = size==1 && _requests.begin()->first==SubscriptionRequestId ? 500ms : 1ms; //UA_CreateSubscriptionRequest_default
				co_await DurationTimer{ sleep };
			}
		}
		if( !sc && !_stopped.test() && _pingInterval.count()>0 )
			ping( _client );
		else
			DBG( "{}ProcessingLoop stopped", logPrefix() );
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