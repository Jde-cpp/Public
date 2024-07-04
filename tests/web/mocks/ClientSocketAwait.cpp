#include "ClientSocketAwait.h"
#include <jde/http/IClientSocketSession.h>

namespace Jde::Http{
/*	ClientSocketMessageAwait::ClientSocketMessageAwait( sp<IClientSocketSession> session, RequestId requestId, string&& request, SL sl )ι:
		_request{ move(request) },
		_requestId{requestId},
		_session{session},
		_sl{sl}
	{}

	α ClientSocketMessageAwait::await_suspend( HClientSocketMessageTask h )ι->void{
		// _tasks.emplace( _requestId, h );
		// _promise = &h.promise();
		// _session->Write( move(_request) );
	}
*/
	//α ClientSocketMessageAwait::await_resume()ε->up<google::protobuf::MessageLite>{
		// ASSERT( _promise );
		//_tasks.erase( _requestId ); TODO
		//return move( _promise->Result );
		//return pResult;
	//}
}