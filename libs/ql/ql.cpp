#include <jde/ql/ql.h>
#include <jde/framework/settings.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLAwait.h>
#include <jde/ql/QLSubscriptions.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/Introspection.h>
#include "LocalQL.h"
#include "MutationAwait.h"

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };
	vector<sp<DB::AppSchema>> _schemas;
  α QueryTables( const vector<TableQL>& tables, UserPK executer, bool log=false, SRCE )ε->jvalue;
	α SetIntrospection( Introspection&& x )ι->void;
//	α Query( RequestQL&& ql, UserPK executer, SL sl )ε->jvalue;
//	α Query( const TableQL& ql, DB::Statement&& statement, UserPK executer, SRCE )ε->jvalue;
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

	α QL::Query( string ql, UserPK executer, SL sl )ε->jvalue{
		Trace{ sl, _tags | ELogTags::Pedantic, "QL: {}", ql };
		return Query( Parse(move(ql)), executer, sl );
	}
	α QL::QueryObject( string ql, UserPK executer, SL sl )ε->jobject{
		let y = Query( ql, executer, sl );
		return y.is_object() ? move( y.get_object() ) : jobject{};
	}
	α QL::QueryArray( string query, UserPK executer, SL sl )ε->jarray{
		let y = Query( move(query), executer, sl );
		ASSERT( y.is_array() );
		return y.is_array() ? move( y.get_array() ) : jarray{};
	}

	α QL::Query( RequestQL&& ql, UserPK executer, SL sl )ε->jvalue{
		vector<TableQL> tableQueries;
		jvalue y;
		if( ql.IsMutation() ){
			bool set{};
			up<IException> e;
			[&]( auto& y, auto& tableQueries, bool& set, auto& e )->MutationAwait::Task {
				let m = move( ql.Mutation() );
				try{
					auto mutationResult = co_await MutationAwait( m, executer, sl );
					if( m.ResultRequest ){
						if( auto array = mutationResult.try_as_array(); array && array->size() )
							mutationResult = Json::AsObject( move((*array)[0]) );
						let available = mutationResult.is_object() ? Json::Combine( m.Args, mutationResult.get_object() ) : move(m.Args);
						Trace{ _tags, "available '{}'", serialize(available) };
						jobject result;
						auto& returnObject = m.ReturnRaw ? result : result[m.CommandName].emplace_object();
						returnObject = m.ResultRequest->TrimColumns( available );
						y = move( result );
					}
				}
				catch( IException& ex ){
					e = ex.Move();
				}
				set = true;
			}( y, tableQueries, set, e );
			while( !set ){
				std::this_thread::yield();
			}
			if( e )
				e->Throw();
		}
		else if( ql.IsTableQL() )
			tableQueries = move( ql.TableQLs() );
		else if( ql.IsSubscription())
			y = jobject{ {"subscriptions", Subscriptions::Add(move(ql.Subscriptions()))} };
		else
			Subscriptions::Remove( move(ql.UnSubscribes()) );

		if( tableQueries.size() )
		y = QueryTables( tableQueries, executer, true, sl );

		Trace{ sl, _tags | ELogTags::Pedantic, "QL::Result: {}", serialize(y) };
		return y;
	}
}

namespace Jde::QL{
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
}
