#include "QuerySessionsAwait.h"
#include "../ServerSocketSession.h"

#define let const auto

namespace Jde::App::Server{
	α QuerySessionsAwait::Suspend()ι->void{
		for( let& session : _sessions )
			SessionQuery( session );
	}
	α QuerySessionsAwait::SessionQuery( sp<ServerSocketSession> session )ι->TAwait<jvalue>::Task{
		uint size;
		try{
			IWebsocketSession& s = dynamic_cast<IWebsocketSession&>( *session );
			auto result = co_await s.QueryClient( _ql, _executer );
			lg _{ _resultsMutex };
			_results.emplace( session->ConnectionPK(), move(result) );
			size = _results.size();
		}
		catch( const std::exception& e ){
			lg _{ _resultsMutex };
			_results.emplace( session->ConnectionPK(), jobject{{"error", e.what()}} );
			size = _results.size();
		}
		if( size == _sessions.size() )
			Resume( move(_results) );
	}
	α QuerySessionsAwait::await_resume()ε->flat_map<ConnectionPK, jvalue>{
		return Promise() ? base::await_resume() : _results;
	}
}