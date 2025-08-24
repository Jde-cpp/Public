#pragma once
#include "../IApp.h"
#include <jde/crypto/CryptoSettings.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/Jwt.h>
#include <jde/app/client/awaits/SocketAwait.h>
#include "AppClientSocketSession.h"

namespace Jde::App::Client{
	struct AppClientSocketSession;
	struct IAppClient : IApp{
		α IsLocal()Ι->bool override{ return false; }
		α UserName()Ι->const jobject&{ return _userName; }
		α SetUserName( jobject&& userName )ι->void{ _userName = move(userName); }
		α UserPK()Ι->Jde::UserPK{ auto p=Session(); return p->UserPK(); }
		α QLServer()ε->sp<QL::IQL>{ auto p=Session(); return p->QLServer(); }
		α PublicKey()Ι->const Crypto::PublicKey& override{ return ServerPublicKey; }

		α SessionInfoAwait( SessionPK sessionPK, SRCE )ι->up<TAwait<Web::FromServer::SessionInfo>> override;
		α SessionInfoAwait( Web::Jwt&& jwt, SRCE )ι->Client::SessionInfoAwait;
		α AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, SRCE )ε->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>;
		α Jwt( SRCE )ε->Web::Client::ClientSocketAwait<Web::Jwt>;
		α CloseSocketSession( SL sl )ι->VoidTask;
		α UpdateStatus()ι->void;

		β StatusDetails()ι->vector<string> = 0;
		optional<Crypto::CryptoSettings> ClientCryptoSettings;
		Crypto::PublicKey ServerPublicKey;
		vector<sp<DB::AppSchema>> SubscriptionSchemas;
	protected:
		α CommonDetails()ι->vector<string>{ return { "TODO" }; }
	private:
		α QueryArray( string&& q, bool returnRaw, SRCE )ε->up<TAwait<jarray>> override;
		α QueryObject( string&& q, bool returnRaw, SRCE )ε->up<TAwait<jobject>> override;
		α QueryValue( string&& q, bool returnRaw, SRCE )ε->up<TAwait<jvalue>> override;
		α SetSession( sp<AppClientSocketSession> session )ι->void{ _session = session; }
		α Session()Ε->sp<AppClientSocketSession>{ auto p = _session; THROW_IF( !p, "Not connected." ); THROW_IF( Process::ShuttingDown(), "Shutting down." ); return p; }

		jobject _userName;
		sp<AppClientSocketSession> _session;

		friend struct AppClientSocketSession; friend struct StartSocketAwait;
	};
}