#include "GatewayQL.h"
#include "GatewayQLAwait.h"
#include <jde/app/client/awaits/LogSettingsClientAwait.h>
#include "../UAClient.h"

namespace Jde::Opc{
	namespace Gateway{ sp<Gateway::GatewayQL> _ql; }
	α Gateway::QLPtr()ι->sp<GatewayQL>{ ASSERT(_ql); return _ql; }
	α Gateway::QL()ι->GatewayQL&{ return *QLPtr(); }
	α Gateway::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return QL().Schemas(); }
	α Gateway::ConfigureQL( sp<DB::AppSchema> schema, sp<Access::Authorize> authorizer )ι->void{
		QL::Configure( {schema} );
		_ql = ms<GatewayQL>( move(schema), move(authorizer) );
	}
}
namespace Jde::Opc::Gateway{
	GatewayQL::GatewayQL( sp<DB::AppSchema>&& schema, sp<Access::Authorize> authorizer )ι:
		App::AppQL{ {schema}, authorizer }{
		QL::Configure( {move(schema)} );
	}
	α GatewayQL::CustomQuery( QL::TableQL& q, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>{
		up<TAwait<jvalue>> await = GatewayQLAwait::Test( q, executer, sl );
		return await;
	}
	α GatewayQL::CustomMutation( QL::MutationQL& m, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>{
		up<TAwait<jvalue>> await = GatewayQLMAwait::Test( m, executer, sl );
		return await;
	}
	α GatewayQL::LogSettingsQuery( QL::TableQL&& ql, QL::Creds executer, SL sl )ε->up<TAwait<jvalue>>{
		RequireAuthenticated( executer, "logSettings", sl );
		return mu<App::Client::LogSettingsClientAwait>( move(ql), sl );
	}
	α GatewayQL::StatusQuery( QL::TableQL&& ql, QL::Creds executer, SL sl )ε->jobject{
		auto y = App::AppQL::StatusQuery( move(ql), executer, sl ); //the base gates it.
		const auto [clients, monitoredItems] = UAClient::StatusCounts();
		y["clients"] = clients;
		y["monitoredItems"] = monitoredItems;
		return y;
	}

}