#pragma once
#ifndef OPC_LOGGER_H
#define OPC_LOGGER_H
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <open62541/server.h>

namespace Jde::Opc{
	//These extend fwk's ELogTags rather than colliding with it:  fwk uses bits 0-26 of the same 64-bit space, so ours
	//start above the 32-bit line.  Opc takes the base bit and the vendor's UA_LogCategory values land in their own
	//order immediately above it, which is what lets UA_Log_Stdout_log map a category to a tag by arithmetic.
	constexpr uint OpcLogTagBase{ 32 };
	constexpr uint UaLogTagBase{ OpcLogTagBase+1 };	//UA_LOGCATEGORY_NETWORK==0 lands here.
	enum class EOpcLogTags : uint{
		Opc           = 1ull << OpcLogTagBase,
		Net           = 1ull << (UaLogTagBase+UA_LOGCATEGORY_NETWORK),
		Secure        = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_SECURECHANNEL),
		Session       = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_SESSION),
		Server        = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_SERVER),
		Client        = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_CLIENT),
		User          = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_USERLAND),
		Security      = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_SECURITYPOLICY),
		EventLoop     = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_EVENTLOOP),
		PubSub        = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_PUBSUB),
		Discovery     = 1ull << (UaLogTagBase+(uint)UA_LOGCATEGORY_DISCOVERY),
		//ours, continuing past the last vendor category (Discovery==9).
		Monitoring    = 1ull << (UaLogTagBase+10),
		Browse        = 1ull << (UaLogTagBase+11),
		ProcessingLoop= 1ull << (UaLogTagBase+12)
	};
	constexpr ELogTags IotReadTag{ ELogTags::Read | (ELogTags)EOpcLogTags::Opc };
	constexpr ELogTags MonitoringTag{ (ELogTags)EOpcLogTags::Monitoring };
	constexpr ELogTags DataChangesTag{ MonitoringTag | ELogTags::Pedantic };
	constexpr ELogTags BrowseTag{ (ELogTags)EOpcLogTags::Browse };
	constexpr ELogTags BrowseTagPedantic{ (ELogTags)EOpcLogTags::Browse | ELogTags::Pedantic };
	constexpr ELogTags ProcessingLoopTag{ (ELogTags)EOpcLogTags::ProcessingLoop };

	struct UALogParser final : Logging::ITagParser{
		α ToTag( str tagName )Ι->ELogTags override;
		α ToString( ELogTags tags )Ι->string override;
		α Tags()Ι->flat_map<string,uint> override;
	};
	struct Logger : UA_Logger{
		Logger( Handle context=0 )ι;
	};
}
#endif