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
  α QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->jvalue;
	α SetIntrospection( Introspection&& x )ι->void;
	Ω query( RequestQL&& ql, UserPK userPK, SL sl )ε->jvalue;
	α Query( const TableQL& ql, DB::Statement&& statement, UserPK userPK )ε->jvalue;

	struct LocalQL final : IQL{
		α Query( string query, UserPK executer, SRCE )ε->QLAwait override{ return QLAwait{ move(query), executer, sl }; }
	};
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

	α QL::Local()ι->sp<IQL>{ return ms<LocalQL>(); }

	α QL::Query( string ql, UserPK userPK, SL sl )ε->jvalue{
		Trace{ sl, _tags | ELogTags::Pedantic, "QL: {}", ql };
		return query( Parse(move(ql)), userPK, sl );
	}
	α QL::QueryObject( string ql, UserPK executer, SL sl )ε->jobject{
		let y = Query( ql, executer, sl );
		return y.is_object() ? move( y.get_object() ) : jobject{};
	}
}

namespace Jde::QL{
	QLAwait::QLAwait( string query, UserPK userPK, SL sl )ε:
		TAwait<jvalue>{sl},
		_request{ Parse(move(query)) },
		_executer{ userPK }
	{}
	QLAwait::QLAwait( TableQL&& ql, DB::Statement&& statement, UserPK userPK, SL sl )ι:
		TAwait<jvalue>{sl},
		_request{ vector<TableQL>{move(ql)} },
		_statement{ move(statement) },
		_executer{ userPK }
	{}

	α QLAwait::Suspend()ι->void{
		CoroutinePool::Resume( _h );
	}

	α QLAwait::await_resume()ε->jvalue{
		jvalue y;
		if( _statement )
			y = Query( get<0>(_request).front(), move(*_statement), _executer );
		else
			y = query( move(_request), _executer, _sl );
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
	α query( RequestQL&& ql, UserPK userPK, SL sl )ε->jvalue{
		vector<TableQL> tableQueries;
		jvalue j;
		if( ql.index()==1 ){
			bool set{};
			up<IException> e;
			[&]( auto& j, auto& tableQueries, bool& set, auto& e )->MutationAwait::Task {
				let m = get<MutationQL>( move(ql) );
				sv resultMemberName = m.Type==EMutationQL::Create ? "id" : "rowCount";
				try{
					auto y = co_await MutationAwait( m, userPK );
					let wantResults = m.ResultPtr && m.ResultPtr->Columns.size()>0;
					if( wantResults && m.ResultPtr->Columns.front().JsonName==resultMemberName && !y.is_null() ){
						jobject jResult;
						jResult[m.JsonName].emplace_object()[resultMemberName] = move(y);
						j = move( jResult );
					}
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
		else
			tableQueries = get<vector<TableQL>>( ql );
		if( tableQueries.size() ){
			j = QueryTables( tableQueries, userPK );
		}

		Trace{ sl, _tags | ELogTags::Pedantic, "QL::Result: {}", serialize(j) };
		return j;
	}
}
