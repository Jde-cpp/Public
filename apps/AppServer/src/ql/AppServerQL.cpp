#include "AppServerQL.h"
#include <jde/access/server/accessServer.h>
#include <jde/app/log/LogQLAwait.h>
#include <jde/app/IApp.h>
#include "AppQLAwait.h"

namespace Jde::App{
	sp<Server::AppServerQL> _ql;
	α Server::QLPtr()ι->sp<QL::LocalQL>{ ASSERT(_ql); return _ql; }
	α Server::QL()ι->QL::LocalQL&{ return *QLPtr(); }
	α Server::ConfigureQL( vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer )ι->void{
		QL::Configure( schemas );
		AppServerQL x( move(schemas), move(authorizer) );
		_ql = ms<AppServerQL>( move(schemas), move(authorizer) );
	}
}

namespace Jde::App::Server{
	AppServerQL::AppServerQL( vector<sp<DB::AppSchema>>&& schemas, sp<Access::Authorize> authorizer )ι:
		AppQL{ move(schemas), move(authorizer) }{
		QL::Configure( move(schemas) );
	}

	α AppServerQL::CustomQuery( QL::TableQL& q, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>{
		auto await = Access::Server::CustomQuery( q, move(executer), sl );
		if( !await )
			await =  AppQLAwait::Test( q, executer, sl );
		return await;
	}
	α AppServerQL::CustomMutation( QL::MutationQL& m, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>{
		auto await = Access::Server::CustomMutation( m, move(executer), sl );
		return await;
	}
}