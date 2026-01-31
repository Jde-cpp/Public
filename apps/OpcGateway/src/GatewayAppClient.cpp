#include "GatewayAppClient.h"
#include "ql/GatewayQLAwait.h"

namespace Jde::Opc{
	static sp<App::Client::IAppClient> _appClient = ms<Gateway::GatewayAppClient>();
	α Gateway::AppClient()ι->sp<App::Client::IAppClient>{ return _appClient; }
}
namespace Jde::Opc::Gateway{
	α GatewayAppClient::ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, bool raw, SL sl )ε->up<TAwait<jvalue>>{
		return mu<GatewayQLAwait>( move(q), executer, raw, sl );
	}
}