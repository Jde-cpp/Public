#include "QuerySessionsAwait.h"
#include "../ServerSocketSession.h"

#define let const auto

namespace Jde::App::Server{
	α QuerySessionsAwait::Suspend()ι->void{
		for( let& session : _sessions )
			SessionQuery( session );
	}
	α QuerySessionsAwait::SessionQuery( sp<ServerSocketSession> session )ι->TAwait<jvalue>::Task{
		uint completed;//count responses independently of _results.size(): a duplicate ConnectionPK (e.g. 0) collapses map entries and would otherwise wedge completion.
		try{
			IWebsocketSession& s = dynamic_cast<IWebsocketSession&>( *session );
			auto result = co_await s.QueryClient( _ql, _executer );
			lg _{ _resultsMutex };
			_results.emplace( session->ConnectionPK(), move(result) );
			completed = ++_completed;
		}
		catch( const std::exception& e ){
			lg _{ _resultsMutex };
			_results.emplace( session->ConnectionPK(), jobject{{"error", e.what()}} );
			completed = ++_completed;
		}
		if( completed == _sessions.size() )
			Resume( move(_results) );
	}
	α QuerySessionsAwait::await_resume()ε->flat_map<ConnectionPK, jvalue>{
		return Promise() ? base::await_resume() : _results;
	}
}