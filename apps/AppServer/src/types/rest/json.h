#pragma once
#include <jde/framework/chrono.h>
namespace Jde::App{

	Ξ ToJson( const Proto::FromClient::Instance& x )ι->jobject{
		return jobject{
			{"application", x.application()},
			{"host", x.host()},
			{"pid", x.pid()},
			{ "startTime", ToIsoString(Jde::Proto::ToTimePoint(x.start_time())) },
			{"port", x.web_port()},
			{"instanceName", x.instance_name()}
		};
	}
}
