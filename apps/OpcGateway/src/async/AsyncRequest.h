#pragma once
#include <jde/fwk/co/Timer.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Logger.h>

namespace Jde::Opc::Gateway{
	constexpr RequestId PingRequestId = 0;
	constexpr RequestId ConnectRequestId = std::numeric_limits<RequestId>::max();
	constexpr RequestId SubscriptionRequestId = ConnectRequestId - 1;
	struct UAClient;
	struct UARequest{
		UARequest( std::any&& h )ι:CoHandle{ move(h) }{ h=nullptr; }
		std::any CoHandle;
	};
	Τ struct UARequestMulti{
		flat_map<UA_UInt32, NodeId> Requests;
		sp<UAClient> ClientPtr;
		flat_map<NodeId, T> Results;
	};

	struct AsyncRequest final{
		α Process( RequestId requestId, sv what )ι->void;
		α Clear( RequestId requestId )ι->void;
		α SetClient( sp<UAClient> client )ι{_client=client;}
		α Stop()ι->void;
		α IsRunning()Ι->bool{ return _running.test(); }
	private:
		α UAHandle()ι->Handle;
		α ProcessingLoop()ι->DurationTimer::Task;
		flat_set<RequestId> _requests; shared_mutex _requestMutex;
		sp<UAClient> _client;
		atomic_flag _running;
		atomic_flag _stopped; //set at shutdown
		constexpr static ELogTags _tags{ (ELogTags)EOpcLogTags::ProcessingLoop };
	};
}