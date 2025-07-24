#include <jde/app/client/appClient.h>
#include <jde/framework/coroutine/Timer.h>
#include <jde/access/Authorize.h>
#include <jde/web/client/socket/ClientQL.h>
#include <jde/app/client/usings.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/IAppClient.h>

#define let const auto


namespace Jde::App::Client{ static sp<Access::Authorize> _authorize; }
namespace Jde::App{
	using Web::Client::ClientHttpAwait;
	const Duration _reconnectWait{ Settings::FindDuration("server/reconnectWait").value_or(5s) };
	α Client::IsSsl()ι->bool{ return Settings::FindBool("server/isSsl").value_or( false ); }
	α Client::Host()ι->string{ return Settings::FindString("server/host").value_or("localhost"); }
	α Client::Port()ι->PortType{ return Settings::FindNumber<PortType>("server/port").value_or(1967); }

	α Client::RemoteAcl( string libName )ι->sp<Access::Authorize>{
		if( !_authorize )
			_authorize = ms<Access::Authorize>( move(libName) );
		return _authorize;
	}
}
namespace Jde::App::Client{
	struct LoginAwait final : TAwait<SessionPK>{
		using base = TAwait<SessionPK>;
		LoginAwait( const Crypto::CryptoSettings& cryptoSettings, const jobject& userName, SRCE )ε;
		α Suspend()ι->void{ Execute(); };
	private:
		α Execute()ι->Web::Client::ClientHttpAwait::Task;
		Web::Jwt _jwt;
	};

	Ω getJwt( const Crypto::CryptoSettings& cryptoSettings, const jobject& userName )ε->Web::Jwt{
		auto pubKey = Crypto::ReadPublicKey( cryptoSettings.PublicKeyPath );
		auto name = Json::FindString( userName, "name" ); THROW_IF( !name, "credentials/name not found in settings." );
		auto target = Json::FindString( userName, "target" ).value_or( *name );
		return Web::Jwt{ pubKey, move(*name), move(target), 0, {}, TimePoint::min(), {}, cryptoSettings.PrivateKeyPath };
	}
	LoginAwait::LoginAwait( const Crypto::CryptoSettings& cryptoSettings, const jobject& userName, SL sl )ε:
		base{sl},
		_jwt{ getJwt(cryptoSettings, userName) }
	{}

	α LoginAwait::Execute()ι->ClientHttpAwait::Task{
		try{
			jobject j{ {"jwt", _jwt.Payload()} };
			Trace{ ELogTags::App, "Logging in {}:{}", Host(), Port()};
			auto res = co_await ClientHttpAwait{ Host(), "/login", serialize(j), Port() };
			auto sessionPK = Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
			THROW_IF( !sessionPK, "Invalid authorization: {}.", res[http::field::authorization] );
			Resume( move(*sessionPK) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	ConnectAwait::ConnectAwait( sp<IAppClient> appClient, jobject userName, bool retry, SL sl )ι:
		VoidAwait<>{sl},
		_appClient{ appClient },
		_retry{retry},
		_userName{ userName }
	{}

	α ConnectAwait::Retry()->DurationTimer::Task{
		co_await DurationTimer{ _reconnectWait };
		HttpLogin();
	}
	α ConnectAwait::RunSocket( SessionPK sessionId )ι->TAwait<Proto::FromServer::ConnectionInfo>::Task{
		try{
			Trace( ELogTags::App, "Creating socket session for sessionId: {:x}", sessionId );
			co_await StartSocketAwait{ sessionId, _authorize, move(_appClient), _sl };

			Resume();
		}
		catch( IException& e ){
			if( _retry )
				Retry();
			else
				ResumeExp( move(e) );
		}
	}
	α ConnectAwait::HttpLogin()ι->LoginAwait::Task{
		try{
			let sessionId = co_await LoginAwait{ *_appClient->ClientCryptoSettings, _userName };//http call
			RunSocket( sessionId );
		}
		catch( IException& e ){
			if( _retry )
				Retry();
			else
				ResumeExp( move(e) );
		}
	}
}