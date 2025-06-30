#include <jde/db/awaits/SelectAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>

namespace Jde::DB{
	α SelectAwait::Execute()ι->QueryAwait::Task{
		try{
			Resume( move((co_await _ds->Query(move(_sql), false, base::_sl)).Rows) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}