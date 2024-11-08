#include <jde/ql/ql.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/Introspection.h>

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };
	vector<sp<DB::AppSchema>> _schemas;
  α Mutation( const MutationQL& m, UserPK userPK )ε->uint;
  α QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->jobject;
	α SetIntrospection( Introspection&& x )ι->void;
}
namespace Jde{
	α QL::Configure( vector<sp<DB::AppSchema>>&& schemas )ε->void{
		_schemas = move( schemas );
		for( let& schema : _schemas ){
			if( let path = Settings::FindSV(schema->ConfigPath()+"/ql"); path ){
				SetIntrospection( {Json::ReadJsonNet(*path)} );
			}
		}
	}
namespace QL{
	Ω query( RequestQL&& ql, UserPK userPK, SL sl )ε->jobject{
		vector<TableQL> tableQueries;
		jobject j;
		if( ql.index()==1 ){
			let& mutation = get<MutationQL>( ql );
			uint result = Mutation( mutation, userPK );
			sv resultMemberName = mutation.Type==EMutationQL::Create ? "id" : "rowCount";
			let wantResults = mutation.ResultPtr && mutation.ResultPtr->Columns.size()>0;
			if( wantResults && mutation.ResultPtr->Columns.front().JsonName==resultMemberName )
				j["data"].emplace_object()[mutation.JsonName].emplace_object()[resultMemberName] = result;
			else if( mutation.ResultPtr )
				tableQueries.push_back( *mutation.ResultPtr );
		}
		else
			tableQueries = get<vector<TableQL>>( ql );
		jobject y = tableQueries.size() ? QueryTables( tableQueries, userPK ) : j;
		Trace{ _tags | ELogTags::Pedantic, "QL::Result: {}", serialize(y) };
		return y;
	}
}
	α QL::Query( string ql, UserPK userPK, SL sl )ε->jobject{
		Trace{ sl, _tags | ELogTags::Pedantic, "QL: {}", ql };
		return query( Parse(move(ql)), userPK, sl );
	}

	α QL::CoQuery( string q_, UserPK u_, SL sl )ι->TPoolAwait<jobject>{
		return Coroutine::TPoolAwait<jobject>( [q=move(q_), u=u_](){
			return mu<jobject>(Query(q,u));
		}, {}, sl );
	}
}

namespace Jde::QL{
	QLAwait::QLAwait( string query, UserPK userPK, SL sl )ε:
		TAwait<jobject>{sl},
		_request{ Parse(move(query)) },
		_userPK{ userPK }
	{}

	α QLAwait::Suspend()ι->void{
		CoroutinePool::Resume( _h );
	}

	α QLAwait::await_resume()ε->jobject{
		return query( move(_request), _userPK, _sl );
	}

	α GetTable( str tableName )ε->sp<DB::View>{
		for( let& schema : _schemas ){
			if( let pTable = schema->GetTablePtr(tableName); pTable )
				return DB::AsView( pTable );
		}
		THROW( "Could not find table '{}'", tableName );
	}
  α Schemas()ι->const vector<sp<DB::AppSchema>>&{
    return _schemas;
  }
}