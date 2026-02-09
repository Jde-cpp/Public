#include "GatewayQL.h"
#include "GatewayQLAwait.h"

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
		BREAK;//if( q.JsonName=="status" ) queryResult = App::IApp::Status();
		up<TAwait<jvalue>> await = GatewayQLAwait::Test( q, executer, sl );
		return await;
	}
	α GatewayQL::CustomMutation( QL::MutationQL& m, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>{
		up<TAwait<jvalue>> await = GatewayQLMAwait::Test( m, executer, sl );
		return await;
	}
}