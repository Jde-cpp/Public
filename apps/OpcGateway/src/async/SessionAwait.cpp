#include "SessionAwait.h"
#include "../UAClient.h"

namespace Jde::Opc::Gateway{
	α SessionAwait::Suspend()ι->void{
		_client->AddSessionAwait( _h );
	}
}