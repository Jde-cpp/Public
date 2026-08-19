#include <jde/db/awaits/SelectAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>

namespace Jde::DB{
	α SelectAwait::await_ready()ι->bool{
		if( !_ds->CompletesInline() )
			return false;
		_inlined = true;
		try{
			_rows = _ds->Select( move(_sql), base::_sl ); //the same Execute() the driver's QueryAwait would have reached.
		}
		catch( Exception& e ){
			_exception = e.Move();
		}
		catch( runtime_error& e ){
			_exception = mu<Exception>( move(e) );
		}
		return true;
	}
	α SelectAwait::await_resume()ε->vector<Row>{
		if( _exception )
			_exception->Throw();
		return _inlined ? move(_rows) : base::await_resume();
	}

	α SelectAwait::Execute()ι->QueryAwait::Task{
		try{
			Resume( move((co_await _ds->Query(move(_sql), false, base::_sl)).Rows) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}