#include "ForwardExecutionAwait.h"
#include <jde/app/proto/app.FromServer.h>
#include "../ServerSocketSession.h"
#include "../WebServer.h"
#define let const auto

namespace Jde::App::Server{
	struct SessionHandle final{
		sp<ServerSocketSession> RequestSocketSession;
		ForwardExecutionAwait::Handle Handle;
		ConnectionPK TargetConnectionPK;//the instance the request was sent to; only it may answer.  The map is keyed by id alone, so without this any client could resolve any client's forward.
	};
	concurrent_flat_map<RequestId,SessionHandle> _forwardExecutionMessages;
	ForwardExecutionAwait::ForwardExecutionAwait( UserPK userPK, Proto::FromClient::ForwardExecution&& customRequest, sp<ServerSocketSession> serverSocketSession, SL sl )ι:
		base{ sl },
		_userPK{ userPK },
		_forwardExecutionMessage{ move(customRequest) },
		_requestSocketSession{ serverSocketSession }
	{}

	α ForwardExecutionAwait::Suspend()ι->void{
		let serverRequestId = Server::NextRequestId();
		let rawInstancePK = _forwardExecutionMessage.app_instance_pk();
		optional<ProgInstPK> instancePK = rawInstancePK ? optional<ProgInstPK>{rawInstancePK} : nullopt;//0 = any instance of the app (App.FromClient.proto); a raw uint32 would never be nullopt, defeating the wildcard.
		ConnectionPK targetConnectionPK{};
		try{
			targetConnectionPK = Server::Write( _forwardExecutionMessage.app_pk(), instancePK, FromServer::ExecuteRequest(serverRequestId, _userPK, move(*_forwardExecutionMessage.mutable_execution_transmission())) );
		}
		catch( runtime_error& e ){ //Could not find app instance.
			ResumeExp( move(e) );//SetExp + h.resume() runs this coroutine's catch to completion and (suspend_never final_suspend) destroys its frame - nothing was added to the map, so no dead handle survives.
			return;
		}
		//Emplace only after a successful Write, keyed to the connection Write actually delivered to (not the wildcard-able app_instance_pk), so only that session can answer (finding #5).  On failure the frame above is already gone; an entry left behind would let a later kExecuteResponse/kException or OnCloseConnection call h.promise() on freed memory (finding #7).
		_forwardExecutionMessages.emplace( serverRequestId, SessionHandle{move(_requestSocketSession), _h, targetConnectionPK} );
	}

	α ForwardExecutionAwait::OnCloseConnection( uint32 requesterId, ConnectionPK targetConnectionPK )ι->void{
		_forwardExecutionMessages.erase_if( [=](auto&& kv){
			let requesterDied = kv.second.RequestSocketSession->Id()==requesterId;//match the requester by its unique socket id, not ConnectionPK - that is 0 for every unregistered web client, so one disconnect would cancel them all.
			let targetDied = targetConnectionPK && kv.second.TargetConnectionPK==targetConnectionPK;//the instance we forwarded to is gone; its reply will never arrive, so fail the forward instead of leaking it (there is no timeout on this path).
			if( !requesterDied && !targetDied )
				return false;
			auto h = kv.second.Handle;
			h.promise().ResumeExp( Exception{targetDied ? "Forward target disconnected." : "Connection closed."}, h );
			return true;
		} );
	}
	α ForwardExecutionAwait::ResumeExp( Exception&& e, RequestId serverRequestId, ConnectionPK responder )ι->bool{
		return _forwardExecutionMessages.erase_if( serverRequestId, [&](auto&& kv){
			if( !responder || kv.second.TargetConnectionPK!=responder )//only the instance the request was sent to may answer; 0 is an unregistered session, never a forward target.
				return false;
			auto h = kv.second.Handle;
			h.promise().ResumeExp( move(e), h );
			return true;
		});
	}
	α ForwardExecutionAwait::Resume( string&& results, RequestId serverRequestId, ConnectionPK responder )ι->bool{
		return _forwardExecutionMessages.erase_if( serverRequestId, [&](auto&& kv){
			if( !responder || kv.second.TargetConnectionPK!=responder )
				return false;
			auto h = kv.second.Handle;
			h.promise().Resume( move(results), h );
			return true;
		});
	}
}