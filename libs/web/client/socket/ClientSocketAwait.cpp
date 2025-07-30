#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/client/socket/IClientSocketSession.h>

namespace Jde::Web::Client{
	α IClientSocketVoidAwait::SessionId()ι->SessionPK{ return _session->Id(); }

	α IClientSocketVoidAwait::Suspend( std::any hCoroutine )ι->void{
		_session->AddTask( _requestId, hCoroutine );
		_session->Write( move(_request) );
	}

	α ClientSocketVoidAwait::await_resume()ε->void{
		AwaitResume();
		auto p = Promise(); THROW_IF( !p, "Not Connected" );
		if( auto e = p->MoveExp(); e )
			e->Throw();
//		p->Log( _session->Id(), _start, _sl );
	}
}