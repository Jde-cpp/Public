#include "HubQL.h"
#include <jde/access/server/accessServer.h>
#include <jde/app/log/LogSettingsAwait.h>
#include <jde/app/client/awaits/LogSettingsClientAwait.h>
#include "../../../AppServer/src/LocalClient.h"
#include "../../../AppServer/src/ql/AppQLAwait.h"
#include "../../../AppServer/src/ql/InstanceTagLevelAwait.h"
#include "../../../OpcGateway/src/ql/GatewayQL.h"
#include "../../../OpcGateway/src/ql/GatewayQLAwait.h"

namespace Jde::Opc::Hub{
	HubQL::HubQL( vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer )ι:
		App::AppQL{ move(schemas), move(authorizer) }
	{}
	α HubQL::CustomQuery( QL::TableQL& q, QL::Creds creds, SL sl )ι->up<TAwait<jvalue>>{
		if( auto await = App::Server::AppQLAwait::Test(q, creds, sl); await )//access custom, connections, settings, instanceTagLevel - nothing the gateway also names.
			return await;
		return Gateway::GatewayQLAwait::IsApplicable(q) ? Gateway::GatewayQLAwait::Test( q, move(creds), sl ) : nullptr;
	}
	α HubQL::CustomMutation( QL::MutationQL& m, QL::Creds creds, SL sl )ι->up<TAwait<jvalue>>{
		if( auto await = Access::Server::CustomMutation(m, creds, sl); await )
			return await;
		if( App::LogSettingsMAwait::IsApplicable(m) )//covers the gateway's plural spelling too; the process is its own AppServer, so the server variant.
			return mu<App::LogSettingsMAwait>( move(m), App::Server::AppClient(), creds.UserPK(), sl );
		if( App::Server::InstanceTagLevelMAwait::IsApplicable(m) )
			return mu<App::Server::InstanceTagLevelMAwait>( move(m), creds.UserPK(), sl );
		return Gateway::GatewayQLMAwait::IsApplicable(m) ? mu<Gateway::GatewayQLMAwait>( move(m), move(creds), sl ) : nullptr;
	}
	α HubQL::LogSettingsQuery( QL::TableQL&& ql, QL::Creds executer, SL sl )ε->up<TAwait<jvalue>>{
		RequireAuthenticated( executer, "logSettings", sl );
		return mu<App::Client::LogSettingsClientAwait>( move(ql), sl );//the gateway's variant: the server's columns plus `appServer`, empty here (no RemoteLog).
	}
	α HubQL::StatusQuery( QL::TableQL&& ql, QL::Creds executer, SL sl )ε->jobject{
		auto y = App::AppQL::StatusQuery( move(ql), executer, sl );//the base gates it.
		Gateway::AddStatusCounts( y );
		return y;
	}
}
