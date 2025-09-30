#include <jde/ql/UnsubscribeAwait.h>
#include <jde/ql/IQL.h>
#include <jde/fwk/str.h>

#define let const auto
namespace Jde::QL{
	α UnsubscribeAwait::Execute()ι->TAwait<jvalue>::Task{
		try{
			co_await *_qlServer->Query( Ƒ("unsubscribe( id:[{}] )", Str::Join(_ids, ",")), {0}, true, _sl );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}