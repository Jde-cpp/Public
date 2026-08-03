#include <jde/ql/QLAwait.h>
#define let const auto

namespace Jde::QL{
	//The jvalue engine: dispatch a RequestQL to the query/mutation awaits, producing a jvalue.
	//The jobject/jarray QLAwait specializations (in QLAwait.h) co_await this and adapt the result.
	template<> α QLAwait<jvalue>::Execute()ι->TAwait<jvalue>::Task{
		try{
			if( _request.IsQueries() )
				Resume( co_await TablesAwait{move(_request.Queries()), move(_statement), _executer, move(_ql), _sl} );
			else if( _request.IsMutation() ){
				jarray mutationResults;
				for( auto& m : _request.Mutations() ){
					LOGSL( ELogLevel::Trace, _sl, ELogTags::QL, "QL: {}", m.ToString() );
					auto resultRequest = m.ResultRequest; // can't move, some mutations may need it
					let returnRaw = m.ReturnRaw;
					let commandName = m.CommandName;
					auto mutationResult = co_await MutationAwait( m, _executer, _ql, _sl );
					if( !resultRequest )
						continue;
					if( auto array = mutationResult.is_array() ? &mutationResult.get_array() : nullptr; array && array->size() )
						mutationResult = Json::AsObject( move((*array)[0]) );
					let available = mutationResult.is_object() ? Json::Combine( m.ExtrapolateVariables(), mutationResult.get_object() ) : m.ExtrapolateVariables();
					jobject result;
					auto& returnObject = returnRaw ? result : result[commandName].emplace_object();
					returnObject = resultRequest->TrimColumns( available );
					mutationResults.push_back( move(result) );
				}
				Resume( mutationResults.size()==1 ? move(mutationResults[0]) : move(mutationResults) );
			}
			else if( _request.IsSubscription() )
				ResumeExp( Exception{"Subscriptions are not supported in this context.", {}, _sl} ); //would need a listener
			else
				ResumeExp( Exception{"Unsubscribe is not supported in this context.", {}, _sl} ); //would need a listener
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}