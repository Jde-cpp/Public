#include <jde/ql/QLAwait.h>

namespace Jde::QL{
	//The jvalue engine:  a RequestQL dispatched to the awaitable for its kind - queries to TablesAwait, mutations to MutationsAwait; a
	//subscription needs a listener, which nothing here has.  The jobject/jarray adapter in QLAwait.h co_awaits this and shapes the result.
	template<> α QLAwait<jvalue>::Execute()ι->TAwait<jvalue>::Task{
		try{
			if( _request.IsQueries() )
				Resume( co_await TablesAwait{move(_request.Queries()), move(_statement), _executer, move(_ql), _sl} );
			else if( _request.IsMutation() )
				Resume( co_await MutationsAwait{move(_request.Mutations()), _executer, move(_ql), _sl} );
			else if( _request.IsSubscription() )
				ResumeExp( Exception{"Subscriptions are not supported in this context.", {}, _sl} ); //would need a listener
			else
				ResumeExp( Exception{"Unsubscribe is not supported in this context.", {}, _sl} ); //would need a listener
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}