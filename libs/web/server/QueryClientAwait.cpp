#include <jde/web/server/QueryClientAwait.h>
#include <jde/web/server/IWebsocketSession.h>

namespace Jde::Web::Server{
	α QueryClientAwait::Suspend()ι->void{
		_session->QueryClient( move(_query), _executer, _h );
	}
}