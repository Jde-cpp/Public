#include <jde/db/awaits/RowAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/IRow.h>


namespace Jde::DB{

	α RowAwait::Suspend()ι->void{
		CoroutinePool::Resume( _h );
	}

	α RowAwait::await_resume()ε->vector<up<IRow>>{
		return _ds->Select( move(_sql), _storedProc, _sl );
	}
}