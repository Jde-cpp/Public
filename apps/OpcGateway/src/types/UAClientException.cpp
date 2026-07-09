#include "UAClientException.h"
#include "../UAClient.h"

namespace Jde::Opc::Gateway{
	UAClientException::UAClientException( StatusCode sc, sp<UAClient> client, RequestId requestId, SL sl, ELogLevel level )ι:
		UAException{ sc, Ƒ("[{:x}.{:x}]{}", client->Handle(), requestId, UAException::Message(sc)), sl, {level} }{
		//TODO! (review #27) Mutating the global client registry inside an exception ctor is surprising and is the double-remove feeder behind #2. The intended owner is the retry path (UAClient::Retry/RetryVoid both RemoveClient for exactly these connection-loss codes) but that path is dead code (#28). This is currently the SOLE live removal on BADSERVERNOTCONNECTED, so it can't simply be deleted. Deferred: relocate removal to an explicit throw-site/handler when the retry path is revived and the client lifecycle is reworked (#3 threading redesign).
		if( sc==UA_STATUSCODE_BADSERVERNOTCONNECTED )
			UAClient::RemoveClient( move(client) );
	}
}