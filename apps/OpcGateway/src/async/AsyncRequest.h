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

	//Serializes all UA_Client access for one client: open62541 clients are not thread-safe, so every UA_Client_* call
	//(run_iterate here, submissions/sync services via UAClient::PostUA) runs on _strand. Process/Clear/Stop are
	//strand-only - cross-thread callers go through UAClient, which dispatches onto the strand with a keep-alive.
	struct AsyncRequest final{
		AsyncRequest()ι;
		α Process( RequestId requestId, sv what )ι->void;//strand-only
		α Clear( RequestId requestId )ι->void;//strand-only
		α SetClient( sp<UAClient> client )ι{_client=client;}//pre-concurrency (UAClient::Connect, before the first Process)
		α Stop()ι->void;//strand-only
		α IsRunning()Ι->bool{ return _running.test(); }
		α IsStopped()Ι->bool{ return _stopped.test(); }
		α Strand()Ι->const boost::asio::strand<boost::asio::io_context::executor_type>&{ return _strand; }
	private:
		α UAHandle()ι->Handle;
		α ProcessingLoop()ι->DurationTimer::Task;
		α Ping( sp<UAClient> client )ι->DurationTimer::Task;
		α CancelPing()ι->void;
		boost::asio::strand<boost::asio::io_context::executor_type> _strand;
		flat_set<RequestId> _requests;//strand-confined
		sp<UAClient> _client;//set pre-concurrency, nulled on the strand by Stop
		TimePoint _lastRequest{};//strand-confined; per-client so one client's traffic doesn't mask another's TTL.
		optional<DurationTimer> _pingTimer;//strand-confined
		atomic_flag _running;
		atomic_flag _stopped; //set at shutdown
		constexpr static ELogTags _tags{ (ELogTags)EOpcLogTags::ProcessingLoop };
	};
}
