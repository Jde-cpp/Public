#pragma once
//The pathological IQL the awaits are driven against:  every custom hook returns null, Schemas() is empty, and nothing opens a data
//source.  Shared by TablesAwaitTests (a handler that consumes the table and still returns null) and MutationAwaitTests (no
//CustomMutation claims a system-named mutation, so it falls through to the stock crud ops).
#include <jde/ql/IQL.h>

namespace Jde::QL::Tests{
	struct NullQL final : IQL{
		α Authorizer()ε->Access::Authorize& override{ throw Exception{"No authorizer."}; }
		α AuthorizerPtr()ε->sp<Access::Authorize> override{ return {}; }
		α CustomQuery( TableQL&, Creds, SL )ι->up<TAwait<jvalue>> override{ ++CustomQueryCount; return nullptr; }
		α CustomMutation( MutationQL&, Creds, SL )ι->up<TAwait<jvalue>> override{ ++CustomMutationCount; return nullptr; }
		α LogQuery( TableQL&& ql, Creds executer, SL )ε->up<TAwait<jvalue>> override{ Consumed = mu<TableQL>( move(ql) ); Received = executer.UserPK(); ++LogQueryCount; return nullptr; }
		α LogSettingsQuery( TableQL&& ql, Creds executer, SL )ε->up<TAwait<jvalue>> override{ Consumed = mu<TableQL>( move(ql) ); Received = executer.UserPK(); ++LogSettingsQueryCount; return nullptr; }
		α StatusQuery( TableQL&& ql, Creds executer, SL )ε->jobject override{ Consumed = mu<TableQL>( move(ql) ); Received = executer.UserPK(); return jobject{ {"up",true} }; }
		α Query( string, jobject, UserPK, bool, SL )ε->up<TAwait<jvalue>> override{ return nullptr; }
		α QueryObject( string, jobject, UserPK, bool, SL )ε->up<TAwait<jobject>> override{ return nullptr; }
		α QueryArray( string, jobject, UserPK, bool, SL )ε->up<TAwait<jarray>> override{ return nullptr; }
		α Subscribe( string&&, jobject, sp<IListener>, UserPK, SL )ε->up<TAwait<vector<SubscriptionId>>> override{ return nullptr; }
		α Upsert( string, jobject, UserPK )ε->jarray override{ return {}; }
		α Schemas()Ι->const vector<sp<DB::AppSchema>>& override{ return _schemas; }

		up<TableQL> Consumed;
		UserPK Received{}; //#6: the three log/status routes used to be handed no credentials at all.
		uint LogQueryCount{};
		uint LogSettingsQueryCount{};
		uint CustomQueryCount{};
		uint CustomMutationCount{};
	private:
		const vector<sp<DB::AppSchema>> _schemas; //empty:  nothing here opens a data source.
	};
}
