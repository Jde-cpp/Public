#include <jde/db/awaits/SelectAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>

namespace Jde::DB{
	α SelectAwait::await_ready()ι->bool{
		return _ds->CompletesInline() && Complete( [&]{ return _ds->Select( move(_sql), base::_sl ); } ); //the same Execute() the driver's QueryAwait would have reached.
	}

	α SelectAwait::Execute()ι->QueryAwait::Task{
		return RunQuery( *this, *_ds, move(_sql), false, base::_sl, []( Result&& r ){ return move(r.Rows); } );
	}
}
