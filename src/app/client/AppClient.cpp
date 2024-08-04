#include <jde/app/client/AppClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/usings.h>
#include "../../../../Framework/source/coroutine/Alarm.h"

#define var const auto

namespace Jde::App{
	using Web::Client::ClientHttpAwait;
	const Duration _reconnectWait{ Settings::Get<Duration>("server/reconnectWait").value_or(5s) };
	α Client::IsSsl()ι->bool{ return Settings::Get<bool>("server/isSsl").value_or( false ); }
	α Client::Host()ι->string{ return Settings::Get("server/host").value_or("localhost"); }
	α Client::Port()ι->PortType{ return Settings::Get<PortType>("server/port").value_or(1967); }

	function<vector<string>()> _statusDetails = []->vector<string>{ return {}; };
  α Client::SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void{ _statusDetails = f; }

	#define IF_OK if( auto pSession = Process::ShuttingDown() ? nullptr : AppClientSocketSession::Instance(); pSession )
	α Client::UpdateStatus()ι->void{// update immediately
		IF_OK
			pSession->Write( FromClient::Status(_statusDetails()) );
	}
}
namespace Jde::App::Client{
	struct LoginAwait final : TAwait<SessionPK>{
		using base = TAwait<SessionPK>;
		LoginAwait( SRCE )ι;
		α await_suspend( base::Handle h )ι->void{ base::await_suspend(h); Execute(); };
	private:
		α Execute()ι->Web::Client::ClientHttpAwait::Task;
		Web::Jwt _jwt;
	};

	α GetJwt()ι->Web::Jwt{
		Crypto::CryptoSettings settings{ "http/ssl" };
		auto [mod,exp] = Crypto::ModulusExponent( settings.PublicKeyPath );
		auto name = Settings::Get("credentials/name").value_or( "" );
		auto target = Settings::Get("credentials/target").value_or( name );
		return Web::Jwt{ move(mod), exp, move(name), move(target), {}, {}, settings.PrivateKeyPath };
	}
	LoginAwait::LoginAwait( SL sl )ι:
		base{sl},
		_jwt{ GetJwt() }
	{}

	α LoginAwait::Execute()ι->ClientHttpAwait::Task{
		try{
			json j{ {"jwt", _jwt.Payload()} };
			auto res = co_await ClientHttpAwait{ Host(), "/CertificateLogin", j.dump(), Port() };
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
	α Client::Connect( bool wait, SL sl )ι->void{
		if( Process::ShuttingDown() )
			return;
		[wait]()->Task{
			if( wait )
				co_await Threading::Alarm::Wait( _reconnectWait );
			[]()->LoginAwait::Task {
				try{
					var sessionId = co_await LoginAwait{};
					[sessionId]()->StartSocketAwait::Task {
						try{
							co_await StartSocketAwait{ sessionId };
						}
						catch( IException& e ){
							Connect( true );
						}
					}();
				}
				catch( IException& e ){
					if( e.Code!=2406168687 )//port=0
						Connect( true );
				}
			}();
		}();
	}
}