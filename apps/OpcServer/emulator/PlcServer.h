#pragma once
#include <jde/opc/pubsub/PubSub.h>

namespace Jde::Opc::Emulator{
	//The emulated PLC's own OPC UA server - what a UA-enabled PLC is.  Holds the same nodeset the OpcServer loads
	//(so its tags mirror the server's) and publishes the contract's fields from those local nodes over PubSub.
	//Headless: `Port` only exists because a UA_Server must bind something; nothing connects to it.
	struct PlcServer final : noncopyable{
		PlcServer( UA_UInt16 port, const fs::path& nodeset, PubSub::Config&& contract, SRCE )ε;
		~PlcServer();
		α Iterate()ι->void;//drives the publisher's timers; call from the emulator's loop thread.
		α FindField( sv name )Ι->optional<uint>;//index into Contract().Fields, or none when the contract does not publish it.
		α Write( uint field, double value )ε->void;//the local node the writer samples.
		α Contract()Ι->const PubSub::Config&{ return _contract; }
		α Port()Ι->UA_UInt16{ return _port; }
	private:
		UA_UInt16 _port;
		PubSub::Config _contract;
		UA_Server* _server{};
		up<PubSub::Writer> _writer;
	};
}
