#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/db/IDataSource.h>

namespace Jde::DB{
	α ExecuteAwait::Execute()ι->QueryAwait::Task{
		return RunQuery( *this, *_ds, move(_sql), false, base::_sl, []( Result&& r ){ return (uint32)r.RowsAffected; } );
	}
}
