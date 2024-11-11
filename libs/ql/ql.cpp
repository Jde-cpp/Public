#include <jde/ql/ql.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/Introspection.h>

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };
	vector<sp<DB::AppSchema>> _schemas;
  α Mutation( const MutationQL& m, UserPK userPK )ε->optional<jvalue>;
  α QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->jobject;
	α SetIntrospection( Introspection&& x )ι->void;
	Ω query( RequestQL&& ql, UserPK userPK, SL sl )ε->jobject;
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

	α QL::Query( string ql, UserPK userPK, SL sl )ε->jobject{
		Trace{ sl, _tags | ELogTags::Pedantic, "QL: {}", ql };
		return query( Parse(move(ql)), userPK, sl );
	}

/*	α QL::CoQuery( string q_, UserPK u_, SL sl )ι->TPoolAwait<jobject>{
		return Coroutine::TPoolAwait<jobject>( [q=move(q_), u=u_](){
			return mu<jobject>(Query(q,u));
		}, {}, sl );
	}*/
}

namespace Jde::QL{
	QLAwait::QLAwait( string query, UserPK userPK, SL sl )ε:
		TAwait<jobject>{sl},
		_request{ Parse(move(query)) },
		_userPK{ userPK }
	{}
	QLAwait::QLAwait( TableQL&& ql, DB::Statement&& statement, UserPK userPK, SL sl )ι:
		TAwait<jobject>{sl},
		_request{ vector<TableQL>{move(ql)} },
		_statement{ move(statement) },
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
			if( let pTable = schema->FindView(tableName); pTable )
				return pTable;
		}
		THROW( "Could not find table '{}'", tableName );
	}
  α Schemas()ι->const vector<sp<DB::AppSchema>>&{
    return _schemas;
  }
	α query( RequestQL&& ql, UserPK userPK, SL sl )ε->jobject{
		vector<TableQL> tableQueries;
		jobject j;
		if( ql.index()==1 ){
			let m = get<MutationQL>( move(ql) );
			sv resultMemberName = m.Type==EMutationQL::Create ? "id" : "rowCount";
			auto result = Mutation( m, userPK );
			let wantResults = m.ResultPtr && m.ResultPtr->Columns.size()>0;
			if( wantResults && m.ResultPtr->Columns.front().JsonName==resultMemberName && result )
				j["data"].emplace_object()[m.JsonName].emplace_object()[resultMemberName] = *result;
			else if( m.ResultPtr )
				tableQueries.push_back( *m.ResultPtr );
		}
		else
			tableQueries = get<vector<TableQL>>( ql );
		jobject y = tableQueries.size() ? QueryTables( tableQueries, userPK ) : j;
		Trace{ sl, _tags | ELogTags::Pedantic, "QL::Result: {}", serialize(y) };
		return y;
	}
}
