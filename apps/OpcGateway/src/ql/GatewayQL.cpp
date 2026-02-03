#include "GatewayQL.h"

namespace Jde::Opc{
	namespace Gateway{ sp<Gateway::GatewayQL> _ql; }
	α Gateway::QLPtr()ι->sp<GatewayQL>{ ASSERT(_ql); return _ql; }
	α Gateway::QL()ι->GatewayQL&{ return *QLPtr(); }
	α Gateway::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return QL().Schemas(); }
	α Gateway::ConfigureQL( sp<DB::AppSchema> schema, sp<Access::Authorize> authorizer )ι->void{
		QL::Configure( {schema} );
		_ql = ms<GatewayQL>( move(schema), authorizer );
	}
}
namespace Jde::Opc::Gateway{
	GatewayQL::GatewayQL( sp<DB::AppSchema>&& schema, sp<Access::Authorize> authorizer )ι:
		QL::LocalQL{ {schema}, authorizer }{
		QL::Configure( {move(schema)} );
	}
	α GatewayQL::CustomQuery( QL::TableQL&, UserPK, SL )ι->up<TAwait<jvalue>>{ return nullptr; }
	α GatewayQL::CustomMutation( QL::MutationQL&, UserPK, SL )ι->up<TAwait<jvalue>>{ return nullptr; }

}