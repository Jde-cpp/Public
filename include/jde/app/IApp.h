#pragma once
//#include "usings.h"
#include <jde/framework/coroutine/Await.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/web/Jwt.h>
#include <jde/app/shared/usings.h>
#include <jde/app/shared/proto/Web.FromServer.pb.h>

namespace Jde::App{
	struct IApp{
		virtual ~IApp()=default;//msvc warning

		β IsLocal()Ι->bool{ return false; }
		α InstancePK()Ι->AppInstancePK{ return _instancePK; }
		β PublicKey()Ι->const Crypto::PublicKey& = 0;
		α SetInstancePK( AppInstancePK pk )ι->void{ _instancePK = pk; }
		β SessionInfoAwait( SessionPK sessionPK, SRCE )ε->up<TAwait<Web::FromServer::SessionInfo>> = 0;
		α Verify( const Web::Jwt& jwt )Ε->void;

		template<class T=jobject> α Query( string&& q, bool returnRaw=true, SRCE )ε->up<TAwait<T>>;
		template<class T=jobject> α QuerySync( string&& q, bool returnRaw=true, SRCE )ε->T;
		template<class T=jobject> α QuerySyncSecure( string&& q, SRCE )ε->T{ return QuerySync<T>(move(q), true, sl); } //TODO
	protected:
		β QueryArray( string&& q, bool returnRaw, SRCE )ε->up<TAwait<jarray>> = 0;
		β QueryObject( string&& q, bool returnRaw, SRCE )ε->up<TAwait<jobject>> = 0;
		β QueryValue( string&& q, bool returnRaw, SRCE )ε->up<TAwait<jvalue>> = 0;
	private:
		AppInstancePK _instancePK{};
	};

	Ŧ IApp::Query( string&& q, bool returnRaw, SL sl )ε->up<TAwait<T>>{
		if constexpr ( std::is_same_v<T, jarray> )
			return QueryArray( move(q), returnRaw, sl );
		else if constexpr ( std::is_same_v<T, jobject> )
			return QueryObject( move(q), returnRaw, sl );
		else if constexpr ( std::is_same_v<T, jvalue> )
			return QueryValue( move(q), returnRaw, sl );
		else
			static_assert( false, "Unsupported type for IApp::Query" );
	}
	Ŧ IApp::QuerySync( string&& q, bool returnRaw, SL sl )ε->T{
		up<TAwait<T>> await = Query<T>( move(q), returnRaw, sl );
		return BlockAwait<TAwait<T>,T>( *await );
	}
	Ξ IApp::Verify( const Web::Jwt& jwt )Ε->void{
		THROW_IF( PublicKey()!=jwt.PublicKey, "Signor not trusted" );
		Crypto::Verify( PublicKey(), jwt.HeaderBodyEncoded, jwt.Signature );
	}
}