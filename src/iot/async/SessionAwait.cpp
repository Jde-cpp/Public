#include <jde/iot/async/SessionAwait.h>
#include <jde/iot/uatypes/UAClient.h>

namespace Jde::Iot{
	α SessionAwait::await_suspend( HCoroutine h )ι->void{
		_client->AddSessionAwait( move(h) );
	}
}