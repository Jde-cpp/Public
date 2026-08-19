#include <jde/db/awaits/QueryAwait.h>

namespace Jde::DB{
	//An inner awaitable that is already complete - sqlite's always is - is taken here rather than awaited, so the caller
	//never suspends.  Suspending only to Resume() inline runs the continuation underneath the caller's own frame, which
	//nested one level per await across a run of consecutive sqlite statements.
	α QueryAwait::await_ready()ι->bool{
		if( !_awaitable->await_ready() )
			return false;
		_inlined = true;
		try{
			_result = _awaitable->await_resume();
		}
		catch( Exception& e ){//virtual Move() keeps the driver's exception type - await_resume rethrows the dynamic type.
			_exception = e.Move();
		}
		catch( runtime_error& e ){
			_exception = mu<Exception>( move(e) );
		}
		return true;
	}
	α QueryAwait::await_resume()ε->Result{
		if( _exception )
			_exception->Throw();
		return _inlined ? move(_result) : base::await_resume();
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