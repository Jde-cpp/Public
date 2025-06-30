#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/db/IDataSource.h>

namespace Jde::DB{

	α ExecuteAwait::Execute()ι->QueryAwait::Task{
		try{
			ResumeScaler( (co_await _ds->Query(move(_sql), false, base::_sl)).RowsAffected );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}