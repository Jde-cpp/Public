#include <jde/ql/QLAwait.h>
#include "MutationAwait.h"
#define let const auto

namespace Jde::QL{
  α QueryTables( const vector<TableQL>& tables, UserPK executer, bool log=false, SRCE )ε->jvalue; //GraphQL.cpp
}
namespace Jde{

	constexpr ELogTags _tags{ ELogTags::QL };

	α QL::Query( RequestQL&& ql, UserPK executer, SL sl )ε->jvalue{
		vector<TableQL> tableQueries;
		jvalue y;
		if( ql.IsMutation() ){
			atomic_flag set;
			up<IException> e;
			[&]( auto& y, auto& tableQueries, atomic_flag& set, auto& e )->MutationAwait::Task {
				jarray mutationResults;
				try{
					for( auto& m : move(ql.Mutations()) ){
						Trace{ sl, ELogTags::QL, "QL: {}", m.ToString() };
						auto mutationResult = co_await MutationAwait( m, executer, sl );
						if( m.ResultRequest ){
							Trace{ ELogTags::Test, "{}", serialize(mutationResult) };
							if( auto array = mutationResult.is_array() ? &mutationResult.get_array() : nullptr; array && array->size() )
								mutationResult = Json::AsObject( move((*array)[0]) );
							let available = mutationResult.is_object() ? Json::Combine( m.Args, mutationResult.get_object() ) : move(m.Args);
							jobject result;
							auto& returnObject = m.ReturnRaw ? result : result[m.CommandName].emplace_object();
							returnObject = m.ResultRequest->TrimColumns( available );
							mutationResults.push_back( result );
						}
					}
					y = mutationResults.size()==1 ? move(mutationResults[0]) : move(mutationResults);
				}
				catch( IException& ex ){
					e = ex.Move();
				}
				set.test_and_set();
				set.notify_all();
			}( y, tableQueries, set, e );
			set.wait( false );
			if( e )
				e->Throw();
		}
		else if( ql.IsTableQL() )
			tableQueries = move( ql.TableQLs() );
		else if( ql.IsSubscription() )
			throw Exception{ sl, "Subscriptions are not supported in this context." }; //would need a listener
		else
			throw Exception{ sl, "Unsubscribe is not supported in this context." }; //would need a listener

		if( tableQueries.size() )
			y = QueryTables( tableQueries, executer, true, sl );

		Trace{ sl, _tags | ELogTags::Pedantic, "QL::Result: {}", serialize(y) };
		return y;
	}
}