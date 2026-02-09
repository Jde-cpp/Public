#include "OpcQL.h"

namespace Jde::Opc{
	sp<Server::OpcQL> _ql;
	α Server::QLPtr()ι->sp<OpcQL>{ ASSERT(_ql); return _ql; }
	α Server::QL()ι->OpcQL&{ return *QLPtr(); }
	α Server::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return QL().Schemas(); }
	α Server::ConfigureQL( sp<DB::AppSchema> schema, sp<Access::Authorize> authorizer )ι->void{
		QL::Configure( {schema} );
		_ql = ms<OpcQL>( move(schema), authorizer );
	}
}

namespace Jde::Opc::Server{
	OpcQL::OpcQL( sp<DB::AppSchema>&& schema, sp<Access::Authorize> authorizer )ι:
		App::AppQL{ {schema}, move(authorizer) }{
		QL::Configure( {move(schema)} );
	}
	α OpcQL::CustomQuery( QL::TableQL&, QL::Creds, SL )ι->up<TAwait<jvalue>>{return nullptr;}
	α OpcQL::CustomMutation( QL::MutationQL&, QL::Creds, SL )ι->up<TAwait<jvalue>>{return nullptr;}
}