#pragma once
#include <jde/fwk/co/Timer.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Logger.h>

namespace Jde::Opc::Gateway{
	constexpr RequestId ConnectRequestId = std::numeric_limits<RequestId>::max();
	constexpr RequestId PingRequestId = ConnectRequestId - 1;
	constexpr RequestId SubscriptionRequestId = ConnectRequestId - 2;
	struct UAClient;

	struct UARequest{
		UARequest( std::any&& h )ι:CoHandle{ move(h) }{ h=nullptr; }
		std::any CoHandle;
	};
	//Τ struct TUARequest : UARequest{TUARequest( T&& args, HCoroutine&& h )ι:UARequest{move(h)}, Args{move(args)}{}T Args;};

	Τ struct UARequestMulti{
		flat_map<UA_UInt32, NodeId> Requests;
		sp<UAClient> ClientPtr;
		flat_map<NodeId, T> Results;
	};

	struct AsyncRequest final{
		Ŧ Process( RequestId requestId, T&& h, sv what )ι->void;
		α Process( RequestId requestId, sv what )ι->void{ Process(requestId, coroutine_handle<>{}, what); }
		Ŧ ClearHandle( RequestId requestId )ι->T;
		α Clear( RequestId requestId )ι->void;
		α SetParent( sp<UAClient> client )ι{_client=client;}
		α Stop()ι->void;
		α IsRunning()Ι->bool{ return _running.test(); }
	private:
		α UAHandle()ι->Handle;
		α ProcessingLoop()ι->DurationTimer::Task;
		flat_map<RequestId, std::any> _requests; mutex _requestMutex;
		sp<UAClient> _client;
		atomic_flag _running;
		atomic_flag _stopped; //set at shutdown
		constexpr static ELogTags _tags{ (ELogTags)EOpcLogTags::ProcessingLoop };
	};


	Ξ AsyncRequest::Clear( RequestId requestId )ι->void{
		TRACE( "[{}.{}]Clearing", hex(UAHandle()), hex(requestId) );
		lg _{_requestMutex};
		if( !_requests.erase(requestId) && requestId!=ConnectRequestId )
			CRITICALT( ProcessingLoopTag, "[{}.{}]Could not find request handle.", hex(UAHandle()), hex(requestId) );
	}
	Ŧ AsyncRequest::ClearHandle( RequestId requestId )ι->T{
		TRACE( "[{}.{}]Clearing", hex(UAHandle()), hex(requestId) );
		T userData;
		lg _{_requestMutex};
		if( auto p = _requests.find(requestId); p!=_requests.end() ){
			try{
				userData = std::any_cast<T>( move(p->second) );
				_requests.erase( p );
			}
			catch( const std::bad_any_cast& e ){
				CRITICALT( ProcessingLoopTag, "[{:x}.{:x}]Bad any cast: {}", UAHandle(), requestId, e.what() );
			}
		}
		else
			CRITICALT( ProcessingLoopTag, "[{:x}.{:x}]Could not find request handle.", UAHandle(), requestId );
		return userData;
	}
	Ŧ AsyncRequest::Process( RequestId requestId, T&& h, sv what )ι->void{
		TRACE( "[{}.{}]Processing: {}", hex(UAHandle()), hex(requestId), what );
		if( _stopped.test() )
			return;
		{
			lg _{_requestMutex};
			_requests.emplace( requestId, std::forward<T>(h) );
		}
		if( !_running.test_and_set() )
			ProcessingLoop();
		h = nullptr;
	}
}