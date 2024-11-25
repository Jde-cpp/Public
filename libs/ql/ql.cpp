#include <jde/ql/ql.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/Introspection.h>
#include "MutationAwait.h"

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };
	vector<sp<DB::AppSchema>> _schemas;
  α QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->jobject;
	α SetIntrospection( Introspection&& x )ι->void;
	Ω query( RequestQL&& ql, UserPK userPK, SL sl )ε->jobject;
	α Query( const TableQL& ql, DB::Statement&& statement, UserPK userPK )ε->jvalue;
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
		TAwait<jvalue>{sl},
		_request{ Parse(move(query)) },
		_userPK{ userPK }
	{}
	QLAwait::QLAwait( TableQL&& ql, DB::Statement&& statement, UserPK userPK, SL sl )ι:
		TAwait<jvalue>{sl},
		_request{ vector<TableQL>{move(ql)} },
		_statement{ move(statement) },
		_userPK{ userPK }
	{}

	α QLAwait::Suspend()ι->void{
		CoroutinePool::Resume( _h );
	}

	α QLAwait::await_resume()ε->jvalue{
		jvalue y;
		if( _statement )
			y = Query( get<0>(_request).front(), move(*_statement), _userPK );
		else
			y = query( move(_request), _userPK, _sl );
		return y;
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
			bool set{};
			up<IException> e;
			[&]( auto& j, auto& tableQueries, bool& set, auto& e )->MutationAwait::Task {
				let m = get<MutationQL>( move(ql) );
				sv resultMemberName = m.Type==EMutationQL::Create ? "id" : "rowCount";
				try{
					auto y = co_await MutationAwait( m, userPK );
					let wantResults = m.ResultPtr && m.ResultPtr->Columns.size()>0;
					if( wantResults && m.ResultPtr->Columns.front().JsonName==resultMemberName && !y.is_null() )
						j[m.JsonName].emplace_object()[resultMemberName] = move(y);
					else if( m.ResultPtr )
						tableQueries.push_back( *m.ResultPtr );
				}
				catch( IException& ex ){
					e = ex.Move();
				}
				set = true;
			}( j, tableQueries, set, e );
			while( !set ){
				std::this_thread::yield();
			}
			if( e )
				e->Throw();
		}
		else{
			tableQueries = get<vector<TableQL>>( ql );
			if( tableQueries.size() )
				j = QueryTables( tableQueries, userPK );
		}
		Trace{ sl, _tags | ELogTags::Pedantic, "QL::Result: {}", serialize(j) };
		return j;
	}
}
