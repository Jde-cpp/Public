#include <jde/web/server/IApplicationServer.h>

namespace Jde::Web::Server{
	App::AppInstancePK _instanceId{};

	α IApplicationServer::InstancePK()ι->App::AppInstancePK{return _instanceId;}
	α IApplicationServer::SetInstancePK( App::AppInstancePK x )ι->void{ _instanceId=x; }
}