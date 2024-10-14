#include <jde/db/await/RowAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/IRow.h>


namespace Jde::DB{

	α RowAwait::Suspend()ι->void{
		CoroutinePool::Resume( _h );
	}

	α RowAwait::await_resume()ι->vector<up<IRow>>{
		return _ds->Select( move(_statement), _sl );
	}
}