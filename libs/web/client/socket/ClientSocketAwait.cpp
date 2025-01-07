#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/client/socket/IClientSocketSession.h>

namespace Jde::Web::Client{
	α IClientSocketVoidAwait::SessionId()ι->SessionPK{ return _session->Id(); }

	α IClientSocketVoidAwait::Suspend( coroutine_handle<> h )ι->void{
		_session->AddTask( _requestId, h );
		_session->Write( move(_request) );
	}

	α ClientSocketVoidAwait::await_resume()ε->void{
		base::AwaitResume();
		typename base::TPromise* p = base::Promise(); THROW_IF( !p, "Not Connected" );
		if( auto e = p->MoveError(); e )
			e->Throw();
		p->Log( _session->Id(), _start, _sl );
	}
}