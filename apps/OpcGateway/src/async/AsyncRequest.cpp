#include "AsyncRequest.h"
#include "../UAClient.h"
#include <jde/framework/process/thread.h>
#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::ProcessingLoop };
//	α AsyncRequest::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }

	// 1 per UAClient
	α AsyncRequest::ProcessingLoop()ι->DurationTimer::Task{
		auto logPrefix = format( "[{:x}]", _pClient->Handle() );
		DBG( "{}ProcessingLoop started", logPrefix );
		while( _running.test() ){
			auto pClient = _pClient;
			auto max = [this]()ι->RequestId { lg _{_requestMutex}; return _requests.empty() ? 0 : _requests.rbegin()->first; };
			let preMax = max();
			if( auto sc = UA_Client_run_iterate(*pClient, 0); sc /*&& (sc!=UA_STATUSCODE_BADINTERNALERROR || i!=0)*/ ){
				ERR( "{}UA_Client_run_iterate returned ({:x}){}", logPrefix, sc, UAException::Message(sc) );
				_running.clear();
				break;
			}
			{
				lg _{_requestMutex};
				if( _requests.empty() ){
					_running.clear();
					break;
				}
			}
			if( preMax==max() ){
				co_await DurationTimer{ 1ms }; //UA_CreateSubscriptionRequest_default
				//std::this_thread::sleep_for( 1ms );
				SetThreadDscrptn( "ProcessingLoop" );
			}
		}

		DBG( "{}ProcessingLoop stopped", logPrefix );
	}

	α AsyncRequest::Stop()ι->void{
		lg _{_requestMutex};
		_stopped.test_and_set();
		_requests.clear();
		_running.clear();
		_pClient=nullptr;
	}
	α AsyncRequest::UAHandle()ι->Handle{ return _pClient ? _pClient->Handle() : 0; }
}