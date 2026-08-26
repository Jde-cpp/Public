#pragma once
#include <jde/opc/pubsub/PubSub.h>

namespace Jde::Opc::Server{
	struct UAServer;
	//The server's Part 14 subscriber, from /opcServer/pubsub - the contract in config/pubsub/*.libsonnet that the PLC
	//emulator publishes with.  Resolves the contract's browse paths against the loaded nodesets, then a PubSub::Reader
	//writes every received field into that node.  Owned by globals beside the UAServer and reset before it; the
	//connection itself dies with UA_Server_delete.
	struct PubSubReader final : noncopyable{
		PubSubReader( UAServer& ua, const jobject& settings, SRCE )ε;
		α Config()Ι->const PubSub::Config&{ return _config; }
	private:
		PubSub::Config _config;
		PubSub::Reader _reader;
	};
}
