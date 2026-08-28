#pragma once
#include <jde/fwk/chrono.h>
namespace Jde::App{

	//What discovery needs, and nothing else: where the instance is, and the name the spa routes by.  This is the shape an
	//anonymous caller gets from /opcGateways & /opcServers - it has to find a gateway before it can log in.
	Ξ ToDiscoveryJson( const Proto::FromClient::Instance& x )ι->jobject{
		return jobject{
			{"host", x.host()},
			{"port", x.web_port()},
			{"instanceName", x.instance_name()}
		};
	}
	//The rest goes only to a caller with a user (appserver-review3 #20): the pid is what stopApplicationInstance signals,
	//the start time fingerprints restarts, and the application name is already implied by the endpoint that was asked for.
	Ξ ToJson( const Proto::FromClient::Instance& x )ι->jobject{
		auto y = ToDiscoveryJson( x );
		y["application"] = x.application();
		y["pid"] = x.pid();
		y["startTime"] = ToIsoString( Protobuf::ToTimePoint(x.start_time()) );
		return y;
	}
}
