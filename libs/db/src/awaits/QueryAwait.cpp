#include <jde/db/awaits/QueryAwait.h>

namespace Jde::DB{
	α QueryAwait::Execute()ι->QueryAwait::Task{
		try{
			Resume( co_await *_awaitable );
		}
		catch( Exception& e ){
			ResumeExp( move(e) );
		}
	}
}