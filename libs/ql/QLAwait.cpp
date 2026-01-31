#include <jde/ql/QLAwait.h>
#define let const auto

namespace Jde::QL{
	α VQLAwait::Select( vector<TableQL>&& tables )ι->TablesAwait::Task{
		try{
			Resume( co_await TablesAwait{move(tables), move(_statement), _executer, move(_ql), _sl} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α VQLAwait::Mutate( vector<MutationQL> mutations )ι->MutationAwait::Task{
		jarray mutationResults;
		try{
			for( auto& m : mutations ){
				LOGSL( ELogLevel::Trace, _sl, ELogTags::QL, "QL: {}", m.ToString() );
				auto resultRequest = move(m.ResultRequest);
				let returnRaw = m.ReturnRaw;
				let commandName = m.CommandName;
				auto mutationResult = co_await MutationAwait( m, _executer, move(_ql), _sl );
				if( !resultRequest )
					continue;
				if( auto array = mutationResult.is_array() ? &mutationResult.get_array() : nullptr; array && array->size() )
					mutationResult = Json::AsObject( move((*array)[0]) );
				let available = mutationResult.is_object() ? Json::Combine( m.Args, mutationResult.get_object() ) : move(m.Args);
				jobject result;
				auto& returnObject = returnRaw ? result : result[commandName].emplace_object();
				returnObject = resultRequest->TrimColumns( available );
				mutationResults.push_back( result );
			}
			Resume( mutationResults.size()==1 ? move(mutationResults[0]) : move(mutationResults) );
		}
		catch( exception& ex ){
			ResumeExp( move(ex) );
		}
	}
	α VQLAwait::Suspend()ι->void{
		if( _request.IsQueries() )
			Select( move(_request.Queries()) );
		else if( _request.IsMutation() )
			Mutate( move(_request.Mutations()) );
		else if( _request.IsSubscription() )
			ResumeExp( Exception{_sl, "Subscriptions are not supported in this context." } ); //would need a listener
		else
			ResumeExp( Exception{_sl, "Unsubscribe is not supported in this context."} ); //would need a listener
	}
}