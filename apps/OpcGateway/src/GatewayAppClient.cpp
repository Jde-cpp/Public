#include "GatewayAppClient.h"
#include <jde/ql/QLAwait.h>
#include "ql/GatewayQL.h"

namespace Jde::Opc{
	static sp<App::Client::IAppClient> _appClient = ms<Gateway::GatewayAppClient>();
	α Gateway::AppClient()ι->sp<App::Client::IAppClient>{ return _appClient; }
	α Gateway::SetAppClient( sp<App::Client::IAppClient> client )ι->void{ _appClient = move(client); }//the displaced client's dtor removes its IShutdown registration.
}
namespace Jde::Opc::Gateway{
	α GatewayAppClient::ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, SL sl )ε->up<TAwait<jvalue>>{
		return mu<QL::QLAwait<>>( move(q), QL::Creds{executer}, Gateway::QLPtr(), sl );
	}
}