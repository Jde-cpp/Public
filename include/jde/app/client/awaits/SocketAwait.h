#pragma once
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/web/client/socket/ClientSocketAwait.h>

namespace Jde::App::Client{

	template<class TProto,class TResult>
	struct SocketAwait : TAwait<TResult>{
		using base = TAwait<TResult>;
		SocketAwait( SL sl )ι:base{sl}{}
		α await_ready()ι->bool final{ return Process::ShuttingDown(); }
		α Suspend()ι->void override final;
		β Execute( sp<AppClientSocketSession> pSession )ι->Web::Client::ClientSocketAwait<TProto>::Task=0;
		α await_resume()ε->TResult final;
	};

	struct ΓAC SessionInfoAwait : SocketAwait<Web::FromServer::SessionInfo,Web::FromServer::SessionInfo>{
		using base = SocketAwait<Web::FromServer::SessionInfo,Web::FromServer::SessionInfo>;
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:base{sl}, _sessionId{sessionId}{}
		α Execute( sp<AppClientSocketSession> session )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task override;
		SessionPK _sessionId;
	};

#define $ template<class TProto,class TResult> α SocketAwait<TProto,TResult>
	$::Suspend()ι->void{
		if( auto pSession = AppClientSocketSession::Instance(); pSession )
			Execute( pSession );
		else
			base::Resume();
	}

	$::await_resume()ε->TResult{
		base::AwaitResume();
		auto p = base::Promise();
		THROW_IF( !p, "Shutting Down" );
		THROW_IF( !p->Value(), "No Connection to AppServer." );
		return move( *p->Value() );
	}
}
#undef $