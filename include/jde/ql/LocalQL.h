#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/QLAwait.h>

namespace Jde::DB{ struct AppSchema; struct IDataSource; }
namespace Jde::QL{
	struct IListener;
	struct LocalQL final : IQL{
		LocalQL( vector<sp<DB::AppSchema>>&& schemas, sp<Access::Authorize> authorizer )ι: _authorizer{authorizer}, _schemas{move(schemas)}{}
		α Authorizer()ε->Access::Authorize&{ return *_authorizer; }
		α AuthorizerPtr()ε->sp<Access::Authorize>{ return _authorizer; }
		α Query( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jvalue>> override{ return mu<QLAwait<jvalue>>( move(query), variables, executer, _schemas, returnRaw, sl ); }
		α QueryObject( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jobject>> override{ return mu<QLAwait<jobject>>( move(query), variables, executer, _schemas, returnRaw, sl ); }
		α QueryArray( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jarray>> override{ return mu<QLAwait<jarray>>( move(query), variables, executer, _schemas, returnRaw, sl ); }
		α Upsert( string query, jobject variables, UserPK executer )ε->jarray override;
		α Subscribe( string&& query, jobject variables, sp<IListener> listener, UserPK executer, SRCE )ε->up<TAwait<vector<SubscriptionId>>> override;

		α DS()ι->DB::IDataSource&;
		α GetTable( str tableName, SRCE )ε->DB::View&;
		α GetTablePtr( str tableName, SRCE )ε->sp<DB::View>;
		α Schemas()Ι->const vector<sp<DB::AppSchema>>&{ return _schemas; }

		template<class T=jobject> α QuerySync( string query, UserPK executer, bool returnRaw=true, SRCE )ε->T;
		private:
			sp<Access::Authorize> _authorizer;
			vector<sp<DB::AppSchema>> _schemas;
	};

	Ŧ LocalQL::QuerySync( string query, UserPK executer, bool returnRaw, SL sl )ε->T{
		using Await=QL::QLAwait<T>;
		return BlockAwait<Await,T>( Await{move(query), executer, _schemas, returnRaw, sl} );
	}
}