#include <jde/iot/async/AsyncRequest.h>
#include <jde/iot/uatypes/UAClient.h>
#define var const auto

namespace Jde::Iot{
	constexpr ELogTags _tag{ (ELogTags)EIotLogTags::ProcessingLoop };
//	α AsyncRequest::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }

	// 1 per UAClient
	α AsyncRequest::ProcessingLoop()ι->Task{
		auto logPrefix = format( "[{:x}]", _pClient->Handle() );
		Debug( _tag, "{}ProcessingLoop started", logPrefix );
		while( _running.test() ){
			auto pClient = _pClient;
			auto max = [this]()ι->RequestId{ lg _{_requestMutex}; return _requests.empty() ? 0 : _requests.rbegin()->first; };
			var preMax = max();
			if( auto sc = UA_Client_run_iterate(*pClient, 0); sc /*&& (sc!=UA_STATUSCODE_BADINTERNALERROR || i!=0)*/ ){
				Error( _tag, "{}UA_Client_run_iterate returned ({:x}){}", logPrefix, sc, UAException::Message(sc) );
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
				co_await Threading::Alarm::Wait( 500ms ); //UA_CreateSubscriptionRequest_default
				Threading::SetThreadDscrptn( "ProcessingLoop" );
			}
		}

		Debug( _tag, "{}ProcessingLoop stopped", logPrefix );
	}

	α AsyncRequest::Process( RequestId requestId, up<UARequest>&& userData )ι->void{
		if( _stopped.test() ) return;
		{
			lg _{_requestMutex};
			_requests.emplace( requestId, move(userData) );
		}
		if( !_running.test_and_set() )
			ProcessingLoop();
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