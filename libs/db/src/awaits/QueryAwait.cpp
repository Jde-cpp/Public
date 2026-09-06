#include <jde/db/awaits/QueryAwait.h>

namespace Jde::DB{
	//An inner awaitable that is already complete - sqlite's always is - is taken here rather than awaited, so the caller
	//never suspends (InlineAwait says why that matters).
	α QueryAwait::await_ready()ι->bool{
		return _awaitable->await_ready() && Complete( [&]{ return _awaitable->await_resume(); } );
	}

	α QueryAwait::Execute()ι->QueryAwait::Task{
		try{
			Resume( co_await *_awaitable );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}