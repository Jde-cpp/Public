#pragma once
#include <open62541/server.h>

namespace Jde::Opc{
	enum class EOpcLogTags : uint{
		Opc				= 1ull << 32,
		Net 			= 1ull << (33+UA_LOGCATEGORY_NETWORK),
		Secure		= 1ull << (33+(uint)UA_LOGCATEGORY_SECURECHANNEL),
		Session		= 1ull << (33+(uint)UA_LOGCATEGORY_SESSION),
		Server		= 1ull << (33+(uint)UA_LOGCATEGORY_SERVER),
		Client		= 1ull << (33+(uint)UA_LOGCATEGORY_CLIENT),
		User			= 1ull << (33+(uint)UA_LOGCATEGORY_USERLAND),
		Security	= 1ull << (33+(uint)UA_LOGCATEGORY_SECURITYPOLICY),
		EventLoop	= 1ull << (33+(uint)UA_LOGCATEGORY_EVENTLOOP),
		PubSub		= 1ull << (33+(uint)UA_LOGCATEGORY_PUBSUB),
		Discovery	    = 1ull << (33+(uint)UA_LOGCATEGORY_DISCOVERY),

		Monitoring    = 1ull << (32+11),
		Browse		    = 1ull << (32+12),
		ProcessingLoop= 1ull << (32+13),
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
	};
	struct Logger : UA_Logger{
		Logger( Handle context=0 )ι;
	};
}