#include <jde/app/client/appClient.h>
#include <jde/fwk/process/execution.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/access/Authorize.h>
#include <jde/access/client/accessClient.h>
#include <jde/web/client/socket/ClientQL.h>
#include <jde/app/client/usings.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/IAppClient.h>
#include <jde/app/client/clientSubscriptions.h>

#define let const auto

namespace Jde::App{
	using Web::Client::ClientHttpAwait;
	α reconnectWait()ι->Duration{ return Settings::FindDuration("/server/reconnectWait").value_or(5s); }
	α Client::IsSsl()ι->bool{ return Settings::FindBool("/server/isSsl").value_or( false ); }
	α Client::Host()ι->string{ return Settings::FindString("/server/host").value_or("localhost"); }
	α Client::Port()ι->PortType{ return Settings::FindNumber<PortType>("/server/port").value_or(1967); }

	Ω reloadAccess( sp<Client::IAppClient> appClient )ι->VoidTask{
		try{
			co_await appClient->ReloadAccess();//on the new session's ClientQL - the one Configure was handed died with the old session.
			INFOT( ELogTags::App|ELogTags::Access, "Reloaded the access snapshot on the new session." );
		}
		catch( runtime_error& e ){
			//Not fatal, and not silent: authorization keeps answering from the snapshot it has, which is what it did before this ran
			//at all - but it is stale, and only a restart or the next reconnect will refresh it.
			WARNT( ELogTags::App|ELogTags::Access, "Could not reload the access snapshot on the new session: {}", e.what() );
		}
	}

	α Client::Connect( sp<IAppClient> appClient )ι->ConnectAwait::Task{
		try{
			if( Process::ShuttingDown() ){
				TRACET( ELogTags::App, "Not reconnecting - shutting down." );
				co_return;
			}
			co_await ConnectAwait{ appClient, true };
			if( Client::Subscriptions::Replay(appClient) && appClient->IsAccessConfigured() )
				reloadAccess( move(appClient) );//a replay means this is a reconnect, so the snapshot has a gap in it the deltas never filled.
		}
		catch( runtime_error& )
		{}
	}
}
namespace Jde::App::Client{
	struct LoginAwait final : TAwait<SessionPK>{
		using base = TAwait<SessionPK>;
		LoginAwait( const Crypto::CryptoSettings& cryptoSettings, SRCE )ε;
		α Suspend()ι->void{ Execute(); };
	private:
		α Execute()ι->Web::Client::ClientHttpAwait::Task;
		Web::Jwt _jwt;
	};

	Ω getJwt( const Crypto::CryptoSettings& cryptoSettings )ε->Web::Jwt{
		auto certificate = Crypto::ReadCertificate( cryptoSettings.Certificate.Path );//sole key material - the jwt derives the public key from it; the server's TrustStore chains it at enrollment.
		const Crypto::Certificate info{ certificate };//the cert is also the identity authority - claims mirror the server's enrollment derivation (name: UPN → email → CN, target: CN) so they can't disagree with what enrollment records.
		auto name = info.Upn.size() ? info.Upn : info.Email.size() ? info.Email : info.CommonName;
		return Web::Jwt{ {}, {0}, move(name), info.CommonName, 0, {}, TimePoint::min(), {}, cryptoSettings.PrivateKey, move(certificate) };
	}
	LoginAwait::LoginAwait( const Crypto::CryptoSettings& cryptoSettings, SL sl )ε:
		base{sl},
		_jwt{ getJwt(cryptoSettings) }
	{}

	α LoginAwait::Execute()ι->ClientHttpAwait::Task{
		try{
			jobject j{ {"jwt", _jwt.Payload()} };
			TRACET( ELogTags::App, "Logging in {}:{}", Host(), Port() );
			auto res = co_await ClientHttpAwait{ Host(), "/login", {}, Port(), {.Authorization= Ƒ("Bearer {}", _jwt.Payload())} };
			auto sessionPK = Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
			THROW_IF( !sessionPK, "Invalid authorization: {}.", res[http::field::authorization] );
			Resume( move(*sessionPK) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	ConnectAwait::ConnectAwait( sp<IAppClient> appClient, bool retry, SL sl )ι:
		VoidAwait{sl},
		_appClient{ appClient },
		_retry{ retry }
	{}

	α ConnectAwait::Retry()ι->DurationTimer::Task{
		try{
			(void)co_await DurationTimer{ reconnectWait() };
			THROW_IF( Process::ShuttingDown(), "Shutting down." );
			HttpLogin();
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α ConnectAwait::RunSocket( SessionPK sessionId )ι->TAwait<Proto::FromServer::ConnectionInfo>::Task{
		try{
			THROW_IF( Process::ShuttingDown(), "Shutting down." );
			TRACET( ELogTags::App, "[{}]Creating socket session", hex(sessionId) );
			auto info = co_await StartSocketAwait{ sessionId, _appClient->Acl(), _appClient, _sl };//null for a client that never authorizes (emulator, soak).
			if( _appClient->ResourceSchema.size() && !info.auth_result() )//the AppServer's TestSchemaAdmin gate on the auth_resource we sent
				WARNT( ELogTags::Access, "AppServer declined to delegate '{}' admin checks to this instance - grant its user Administer on the schema's root resources and reconnect;  until then the AppServer applies its flat rule.", _appClient->ResourceSchema );
			_appClient->SetAppPKs( info.instance_pk(), info.connection_pk() );
			Post( _h );  //in OnRead, will block subsequent reads
		}
		catch( runtime_error& e ){
			if( _retry && !Process::ShuttingDown() )
				Retry();
			else
				ResumeExp( move(e) );
		}
	}
	α ConnectAwait::HttpLogin()ι->LoginAwait::Task{
		try{
			let sessionId = co_await LoginAwait{ *_appClient->SslSettings };//http call
			THROW_IF( Process::ShuttingDown(), "Shutting down." );
			RunSocket( sessionId );
		}
		catch( runtime_error& e ){
			if( _retry && !Process::ShuttingDown() )//a retry timer armed during teardown only delays the executor drain.
				Retry();
			else
				ResumeExp( move(e) );
		}
	}
}