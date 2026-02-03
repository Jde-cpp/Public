#include "AsyncRequest.h"
#include "../UAClient.h"
#include <jde/fwk/process/execution.h>
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
		auto client = _client;
		while( _running.test() ){
			uint size;
			{
				sl _{ _requestMutex };
				if( size = _requests.size(); size ){
					if( *_requests.begin()==PingRequestId )
						_requests.erase( PingRequestId );
					else
						_lastRequest = Clock::now();
				}
			}
			TRACE( "{}run_iterate: requestCount: {}", logPrefix(), size );
			if( sc = UA_Client_run_iterate(*client, 0); sc ){
				_running.clear();
				ul _{ _requestMutex };
				let level = _requests.size()>0 ? ELogLevel::Critical : ELogLevel::Debug;
				LOG( level, _tags, "{}UA_Client_run_iterate returned ({:x}){}, requestCount: {}", logPrefix(), sc, UAException::Message(sc), _requests.size() );
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
				TRACE( "{}requestCount: {}, [0]={}", logPrefix(), newSize, hex(*_requests.begin()) );
				_requestMutex.unlock();
			}
			if( size==newSize ){
				let sleep = size==1 && *_requests.begin()==SubscriptionRequestId ? 500ms : 1ms; //UA_CreateSubscriptionRequest_default
				co_await DurationTimer{ sleep };
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
		{
			ul _{_requestMutex};
			_requests.emplace( requestId );
		}
		if( !_running.test_and_set() )
			Post( std::bind(&AsyncRequest::ProcessingLoop, this) );
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