#pragma once
#include <jde/web/server/Sessions.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/web/client/ClientSocketAwait.h>

namespace Jde::App::Client{

	template<class TProto,class TResult>
	struct SocketAwait : TAwait<TResult>{
		using base = TAwait<TResult>;
		SocketAwait( SL sl )ι:base{sl}{}
		α await_ready()ι->bool final{ return IApplication::ShuttingDown(); }
		α await_suspend( base::Handle h )ι->void final;
		β Execute( sp<AppClientSocketSession> pSession )ι->Web::Client::ClientSocketAwait<TProto>::Task=0;
		α await_resume()ε->TResult final;
	};

	struct GraphQLAwait : SocketAwait<string,json>{
		using base = SocketAwait<string,json>;
		GraphQLAwait( string&& q, UserPK userPK, SRCE )ι:base{sl}, _query{q}, _userPK{userPK}{}
		α Execute( sp<AppClientSocketSession> pSession )ι->Web::Client::ClientSocketAwait<string>::Task override;
		string _query; UserPK _userPK;
	};

	struct SessionInfoAwait : SocketAwait<Proto::FromServer::SessionInfo,Web::Server::SessionInfo>{
		using base = SocketAwait<Proto::FromServer::SessionInfo,Web::Server::SessionInfo>;
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:base{sl}, _sessionId{sessionId}{}
		α Execute( sp<AppClientSocketSession> pSession )ι->Web::Client::ClientSocketAwait<Proto::FromServer::SessionInfo>::Task override;
		SessionPK _sessionId;
	};

#define $ template<class TProto,class TResult> α SocketAwait<TProto,TResult>
	$::await_suspend( base::Handle h )ι->void{
		base::await_suspend( h );
		if( auto pSession = AppClientSocketSession::Instance(); pSession )
			Execute( pSession );
		else
			h.resume();
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