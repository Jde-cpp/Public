#include "UAClientException.h"
#include "../UAClient.h"

namespace Jde::Opc::Gateway{
	UAClientException::UAClientException( StatusCode sc, sp<UAClient> client, RequestId requestId, SL sl, ELogLevel level )ι:
		UAException{ sc, Ƒ("[{:x}.{:x}]{}", client->Handle(), requestId, UAException::Message(sc)), sl, {level} }{
		if( sc==UA_STATUSCODE_BADSERVERNOTCONNECTED )
			UAClient::RemoveClient( move(client) );
	}
}