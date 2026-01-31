#include "AppQL.h"
#include <jde/access/server/accessServer.h>

namespace Jde::App{
	sp<Server::AppQL> _ql;
	α Server::QLPtr()ι->sp<QL::LocalQL>{ ASSERT(_ql); return _ql; }
	α Server::QL()ι->QL::LocalQL&{ return *QLPtr(); }
	α Server::ConfigureQL( vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer )ι->void{
		QL::Configure( schemas );
		_ql = ms<AppQL>( move(schemas), authorizer );
	}
}

namespace Jde::App::Server{
	AppQL::AppQL( vector<sp<DB::AppSchema>>&& schemas, sp<Access::Authorize> authorizer )ι:
		QL::LocalQL{ schemas, authorizer }{
		QL::Configure( move(schemas) );
	}

	α AppQL::CustomQuery( QL::TableQL& q, UserPK executer, SL sl )ι->up<TAwait<jvalue>>{
		up<TAwait<jvalue>> await = nullptr;
		await = Access::Server::CustomQuery( q, executer, sl );
		return await;
	}
	α AppQL::CustomMutation( QL::MutationQL& m, UserPK executer, SL sl )ι->up<TAwait<jvalue>>{
		up<TAwait<jvalue>> await = nullptr;
		await = Access::Server::CustomMutation( m, executer, sl );
		return await;
	}
}