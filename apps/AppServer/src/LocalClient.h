#pragma once
#include <jde/ql/QLAwait.h>
#include <jde/app/IApp.h>
#include "WebServer.h"

namespace Jde::App::Server{
	struct LocalClient final : IApp{
		α InitLogging()ι->void;
		α IsLocal()Ι->bool override{ return true; }
		α QueryArray( string&& q, jobject variables, bool raw, SRCE )ι->up<TAwait<jarray>> override{ return mu<QL::QLAwait<jarray>>( move(q), variables, UserPK{UserPK::System}, Server::Schemas(), raw, sl ); }
		α QueryObject( string&& q, jobject variables, bool returnRaw, SRCE )ι->up<TAwait<jobject>> override{ return mu<QL::QLAwait<jobject>>( move(q), variables, UserPK{UserPK::System}, Server::Schemas(), returnRaw, sl ); }
		α QueryValue( string&& q, jobject variables, bool returnRaw, SRCE )ι->up<TAwait<jvalue>> override{ return mu<QL::QLAwait<jvalue>>( move(q), variables, UserPK{UserPK::System}, Server::Schemas(), returnRaw, sl ); }
		α SessionInfoAwait( SessionPK, SL )ι->up<TAwait<Web::FromServer::SessionInfo>> override{ return {}; }
		α PublicKey()Ι->const Crypto::PublicKey& override{ return _publicKey; }
		α SetPublicKey( Crypto::PublicKey publicKey )ι->void{ _publicKey = move(publicKey); }
	private:
		Crypto::PublicKey _publicKey;
	};
	α AppClient()ι->sp<LocalClient>;
}