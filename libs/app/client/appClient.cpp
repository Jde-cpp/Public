#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/usings.h>
#include <jde/access/Authorize.h>
#include <jde/web/client/socket/ClientQL.h>
#include "../../../../Framework/source/coroutine/Alarm.h"

#define let const auto

namespace Jde::App{
	using Web::Client::ClientHttpAwait;

	sp<Access::IAcl> _authorizer = ms<Access::Authorize>();
	const Duration _reconnectWait{ Settings::FindDuration("server/reconnectWait").value_or(5s) };
	α Client::IsSsl()ι->bool{ return Settings::FindBool("server/isSsl").value_or( false ); }
	α Client::Host()ι->string{ return Settings::FindString("server/host").value_or("localhost"); }
	α Client::Port()ι->PortType{ return Settings::FindNumber<PortType>("server/port").value_or(1967); }

	function<vector<string>()> _statusDetails = []()->vector<string>{ return {}; };
  α Client::SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void{ _statusDetails = f; }

	α Client::RemoteAcl()ι->sp<Access::IAcl>{ return _authorizer; }
	#define IF_OK if( auto pSession = Process::ShuttingDown() ? nullptr : AppClientSocketSession::Instance(); pSession )
	α Client::UpdateStatus()ι->void{// update immediately
		IF_OK
			pSession->Write( FromClient::Status(_statusDetails()) );
	}
	α Client::AppServiceUserPK()ι->UserPK{
		//AppClientSocketSession a{ nullptr, nullopt };
		let session = AppClientSocketSession::Instance();
		return session ? session->UserPK() : UserPK{};
	}
	α Client::QLServer()ε->sp<QL::IQL>{
		auto session = Process::ShuttingDown() ? nullptr : AppClientSocketSession::Instance();
		THROW_IF( !session, "Not connected." );
		return session->QLServer();
	};
}
namespace Jde::App::Client{
	struct LoginAwait final : TAwait<SessionPK>{
		using base = TAwait<SessionPK>;
		LoginAwait( SRCE )ε;
		α Suspend()ι->void{ Execute(); };
	private:
		α Execute()ι->Web::Client::ClientHttpAwait::Task;
		Web::Jwt _jwt;
	};

	Ω getJwt()ε->Web::Jwt{
		Crypto::CryptoSettings settings{ "http/ssl" };
		auto [mod,exp] = Crypto::ModulusExponent( settings.PublicKeyPath );
		auto name = Settings::FindString("/credentials/name"); THROW_IF( !name, "credentials/name not found in settings." );
		auto target = Settings::FindString("/credentials/target").value_or( *name );
		return Web::Jwt{ move(mod), exp, move(*name), move(target), {}, {}, settings.PrivateKeyPath };
	}
	LoginAwait::LoginAwait( SL sl )ε:
		base{sl},
		_jwt{ getJwt() }
	{}

	α LoginAwait::Execute()ι->ClientHttpAwait::Task{
		try{
			jobject j{ {"jwt", _jwt.Payload()} };
			auto res = co_await ClientHttpAwait{ Host(), "/CertificateLogin", serialize(j), Port() };
			auto sessionPK = Str::TryTo<SessionPK>( res[http::field::authorization], nullptr, 16 );
			THROW_IF( !sessionPK, "Invalid authorization: {}.", res[http::field::authorization] );
			Resume( move(*sessionPK) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}
namespace Jde::App{
	α Client::Connect( bool wait )ι->void{
		if( Process::ShuttingDown() )
			return;
		[wait]()->Task{
			if( wait )
				co_await Threading::Alarm::Wait( _reconnectWait );
			[]()->LoginAwait::Task {
				SessionPK sessionId;
				try{
					sessionId = co_await LoginAwait{};//http call
				}
				catch( IException& e ){
					Trace( ELogTags::App, "Could not login to App Server:  {}", e.what() );
					if( e.Code!=2406168687 )//port=0
						Connect( true );
				}
				if( sessionId ){
					[sessionId]()->StartSocketAwait::Task {
						try{
							co_await StartSocketAwait{ sessionId };//create socket.
						}
						catch( IException& ){
							Connect( true );
						}
					}();
				}
			}();
		}();
	}
}