#include <jde/iot/async/SessionAwait.h>
#include <jde/iot/uatypes/UAClient.h>

namespace Jde::Iot{
	α SessionAwait::Suspend()ι->void{
		_client->AddSessionAwait( _h );
	}
}