#pragma once
#include <jde/iot/uatypes/Node.h>
#include "../uatypes/Logger.h"

namespace Jde::Iot{
	struct UAClient;

	struct UARequest{
		UARequest( HCoroutine&& h )ι:CoHandle{ move(h) }{}
		virtual ~UARequest(){ /*DBG("~UARequest({:x})", (uint)this);*/ }
//		virtual ~UARequest(){};
		HCoroutine CoHandle;
	};
	//Τ struct TUARequest : UARequest{TUARequest( T&& args, HCoroutine&& h )ι:UARequest{move(h)}, Args{move(args)}{}T Args;};

	Τ struct UARequestMulti{
		flat_map<UA_UInt32, NodeId> Requests;
		sp<UAClient> ClientPtr;
		flat_map<NodeId, T> Results;
	};

	struct AsyncRequest final : boost::noncopyable{
		~AsyncRequest(){};
		α SetParent( sp<UAClient> pClient )ι{_pClient=pClient;}
		α Process( RequestId requestId, up<UARequest>&& userData )ι->void;
		Ŧ ClearRequest( RequestId requestId )ι->up<T>;
		α Stop()ι->void;
	private:
		α UAHandle()ι->Handle;
		//α LogTag()ι->sp<LogTag>;
		α ProcessingLoop()ι->Task;
		flat_map<RequestId, up<UARequest>> _requests; mutex _requestMutex;
		sp<UAClient> _pClient;
		atomic_flag _running;
		atomic_flag _stopped;
	};

	Ŧ AsyncRequest::ClearRequest( RequestId requestId )ι->up<T>{
		up<T> userData;
		lg _{_requestMutex};
		if( auto p = _requests.find(requestId); p!=_requests.end() ){
			userData.reset( dynamic_cast<T*>(p->second.release()) );
			_requests.erase( p );
		}
		else
			Critical( ProcessingLoopTag, "[{:x}.{:x}]Could not find request handle.", UAHandle(), requestId );
		return userData;
	}
}