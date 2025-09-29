#pragma once
#include <jde/framework/co/Timer.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Logger.h>

namespace Jde::Opc::Gateway{
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
		Ŧ Process( RequestId requestId, T&& h )ι->void;
		α Process( RequestId requestId )ι->void{ Process(requestId, coroutine_handle<>{}); }
		Ŧ ClearHandle( RequestId requestId )ι->T;
		α Clear( RequestId requestId )ι->void;
		α SetParent( sp<UAClient> pClient )ι{_pClient=pClient;}
		α Stop()ι->void;
	private:
		α UAHandle()ι->Handle;
		α ProcessingLoop()ι->DurationTimer::Task;
		flat_map<RequestId, std::any> _requests; mutex _requestMutex;
		sp<UAClient> _pClient;
		atomic_flag _running;
		atomic_flag _stopped;
	};


	Ξ AsyncRequest::Clear( RequestId requestId )ι->void{
		lg _{_requestMutex};
		if( !_requests.erase(requestId) )
			CRITICALT( ProcessingLoopTag, "[{:x}.{:x}]Could not find request handle.", UAHandle(), requestId );
	}
	Ŧ AsyncRequest::ClearHandle( RequestId requestId )ι->T{
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
	Ŧ AsyncRequest::Process( RequestId requestId, T&& h )ι->void{
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