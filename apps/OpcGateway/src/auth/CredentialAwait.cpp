#include "CredentialAwait.h"

namespace Jde::Opc::Gateway{
	α CredentialAwait::await_ready()ι->bool{
		_readyResult = GetCredential( _sessionId, _opcId );
		return _readyResult.has_value();
	}

	α CredentialAwait::Suspend()ι->void{

	}
}