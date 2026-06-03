#include "ServerMock.h"
#include "jde/fwk.h"
#include <jde/app/IApp.h>
#include <jde/web/server/Server.h>

namespace Jde::Web{
	optional<std::jthread> _webThread;

	struct TestAppClient final : App::IApp{
		α IsLocal()Ι->bool override{ return true; }
		α GraphQL( string&&, UserPK, bool, SL )ι->up<TAwait<jvalue>>{ return {}; }
		α Login( Web::Jwt&&, SL )ε->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo> override{ throw "noImpl"; }
		α ClientQuery( QL::RequestQL&&, UserPK, SL )ε->up<TAwait<jvalue>> override{ ASSERT(false); return {}; }
		α SessionInfoAwait( SessionPK, SL )ι->up<TAwait<Web::FromServer::SessionInfo>> override{ return {}; }
		α PublicKey()Ι->const Crypto::PublicKey& override{ return _publicKey; }

		α QueryArray( string&&, jobject, bool, SL=SRCE_CUR )ε->up<TAwait<jarray>> override{ return {}; }
		α QueryObject( string&&, jobject, bool, SL=SRCE_CUR )ε->up<TAwait<jobject>> override{ return {}; }
		α QueryValue( string&&, jobject, bool, SL=SRCE_CUR )ε->up<TAwait<jvalue>> override{ return {}; }
	private:
		Crypto::PublicKey _publicKey;
	};
	sp<App::IApp> _appClient = ms<TestAppClient>();
	α Mock::AppClient()ι->sp<App::IApp>{ return _appClient; }

	sp<Mock::RequestHandler> _requestHandler;
	α Mock::Start( jobject settings )ε->void{
		_requestHandler = ms<RequestHandler>( move(settings) );
		Server::Start( _requestHandler );
	}

	α Mock::Stop()ι->void{
		Server::Stop( _requestHandler );
	}
}