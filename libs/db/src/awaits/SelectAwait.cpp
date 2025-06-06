#include <jde/db/awaits/SelectAwait.h>
#include <jde/db/IDataSource.h>

namespace Jde::DB{
	α SelectAwait::Execute()ι->QueryAwait::Task{
		try{
			Resume( move((co_await _ds->Query(move(_sql), base::_sl)).Rows) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}
