#include <jde/opc/async/SessionAwait.h>
#include <jde/opc/uatypes/UAClient.h>

namespace Jde::Opc{
	α SessionAwait::Suspend()ι->void{
		_client->AddSessionAwait( _h );
	}
}