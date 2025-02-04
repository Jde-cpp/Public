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
			bool set{};
			up<IException> e;
			[&]( auto& y, auto& tableQueries, bool& set, auto& e )->MutationAwait::Task {
				jarray mutationResults;
				try{
					for( auto& m : move(ql.Mutations()) ){
						Trace{ ELogTags::QL, "QL {}", m.ToString() };
						auto mutationResult = co_await MutationAwait( m, executer, sl );
						if( m.ResultRequest ){
							if( auto array = mutationResult.try_as_array(); array && array->size() )
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