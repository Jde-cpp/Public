﻿#pragma once

namespace Jde::Iot{
	enum class EIotLogTags : uint{
		Iot				= 1ull << 32,
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
	constexpr ELogTags IotReadTag{ ELogTags::Read | (ELogTags)EIotLogTags::Iot };
	constexpr ELogTags MonitoringTag{ (ELogTags)EIotLogTags::Monitoring };
	constexpr ELogTags DataChangesTag{ MonitoringTag | ELogTags::Pedantic };
	constexpr ELogTags BrowseTag{ (ELogTags)EIotLogTags::Browse };
	constexpr ELogTags BrowseTagPedantic{ (ELogTags)EIotLogTags::Browse | ELogTags::Pedantic };
	constexpr ELogTags ProcessingLoopTag{ (ELogTags)EIotLogTags::ProcessingLoop };

	ΓI α LogTagParser( sv name )ι->optional<ELogTags>;
	struct Logger : UA_Logger{
		Logger( Handle context=0 )ι;

	};
}