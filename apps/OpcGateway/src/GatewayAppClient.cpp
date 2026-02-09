#include "GatewayAppClient.h"
#include <jde/ql/QLAwait.h>

namespace Jde::Opc{
	static sp<App::Client::IAppClient> _appClient = ms<Gateway::GatewayAppClient>();
	α Gateway::AppClient()ι->sp<App::Client::IAppClient>{ return _appClient; }
}
namespace Jde::Opc::Gateway{
	α GatewayAppClient::ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, SL sl )ε->up<TAwait<jvalue>>{
		return mu<QL::QLAwait<>>( move(q), QL::Creds{executer}, sl );
	}
}