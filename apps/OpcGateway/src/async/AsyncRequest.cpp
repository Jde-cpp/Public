#include "AsyncRequest.h"
#include "../UAClient.h"
#include <jde/fwk/process/thread.h>
#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::ProcessingLoop };
	Duration _pingInterval;
	Duration _ttl;
	TimePoint _lastRequest;
	optional<DurationTimer> _pingTimer;

	Ω ping( sp<UAClient> client )ι->DurationTimer::Task{
		ASSERT( !_pingTimer )
		_pingTimer.emplace( _pingInterval, SRCE_CUR );
		try{
			co_await *_pingTimer;
			client->Process( PingRequestId );
		}
		catch( const exception& e )
		{}
	}
	// 1 per UAClient
	α AsyncRequest::ProcessingLoop()ι->DurationTimer::Task{
		function<string()> logPrefix = [this](){ return Ƒ( "[{:x}]", _client->Handle() ); };
		DBG( "{}ProcessingLoop started", logPrefix() );
		_pingTimer.reset();
		while( _running.test() ){
			auto client = _client;
			optional<RequestId> preMax, postMax;
			{
				lg _{_requestMutex};
				if( _requests.size() ){
					preMax = _requests.rbegin()->first;
					if( *preMax==PingRequestId )
						_requests.erase( PingRequestId );
					else
						_lastRequest = Clock::now();
				}
			}
			if( auto sc = UA_Client_run_iterate(*client, 0); sc ){
				ERR( "{}UA_Client_run_iterate returned ({:x}){}", logPrefix(), sc, UAException::Message(sc) );
				_running.clear();
				lg _{_requestMutex};
				_requests.clear();
				break;
			}
			{
				lg _{_requestMutex};
				if( _requests.empty() ){
					_running.clear();
					if( let now = Clock::now(); _lastRequest + _ttl < now ){
						DBG( "{}No requests for {}, shutting down client.", logPrefix(), Chrono::ToString(now-_lastRequest) );
						client->Shutdown( false );
					}
					break;
				}
				postMax = _requests.rbegin()->first;
			}
			if( preMax==postMax )
				co_await DurationTimer{ 1ms }; //UA_CreateSubscriptionRequest_default
		}
		if( !_stopped.test() && _pingInterval.count()>0 )
			ping( _client );
		DBG( "{}ProcessingLoop stopped", logPrefix() );
	}

	α AsyncRequest::Stop()ι->void{
		lg _{_requestMutex};
		_pingTimer.reset();
		_stopped.test_and_set();
		_requests.clear();
		_running.clear();
		_client=nullptr;
	}
	α AsyncRequest::UAHandle()ι->Handle{ return _client ? _client->Handle() : 0; }
}