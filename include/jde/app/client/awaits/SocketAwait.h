#pragma once
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/Jwt.h>

namespace Jde::App::Client{

	template<class TProto,class TResult>
	struct SocketAwait : TAwait<TResult>{
		using base = TAwait<TResult>;
		SocketAwait( sp<AppClientSocketSession>&& session, SL sl )ι:base{sl}, _session{move(session)}{}
		α await_ready()ι->bool final{ return Process::ShuttingDown() || _session==nullptr; }
		α Suspend()ι->void override final{ Execute(); }
		β Execute()ι->Web::Client::ClientSocketAwait<TProto>::Task=0;
		α await_resume()ε->TResult final;
	protected:
		sp<AppClientSocketSession> _session;
	};

	struct ΓAC SessionInfoAwait : SocketAwait<Web::FromServer::SessionInfo,Web::FromServer::SessionInfo>{
		using base = SocketAwait<Web::FromServer::SessionInfo,Web::FromServer::SessionInfo>;
		SessionInfoAwait( SessionPK credentials, sp<AppClientSocketSession> session, SL sl )ι:base{move(session), sl}, _credentials{credentials}{}
	private:
		α Execute()ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task override;
		SessionPK _credentials;
	};

	template<class TProto,class TResult>
	α SocketAwait<TProto,TResult>::await_resume()ε->TResult{
		base::CheckException();
		auto p = base::Promise();
		THROW_IF( !p && !_session, "Shutting Down" );
		THROW_IF( !_session || !p->Value(), "No Connection to AppServer." );
		return move( *p->Value() );
	}
}