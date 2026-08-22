#include "AppInstanceHook.h"
#include <jde/ql/types/TableQL.h>
#include "../WebServer.h"
#include "../ServerSocketSession.h"

#define let const auto

namespace Jde::App::Server{
	struct StartAwait : TAwait<jvalue>{
		StartAwait( QL::MutationQL /*mutation*/, UserPK, SL sl )ι:
			TAwait<jvalue>{ sl }
		{}
		//Resume with the exception rather than leaving Suspend empty: an empty Suspend never resumes _h, so the caller's
		//`co_await *awaitable` hangs forever and the frame leaks.  The base await_resume then rethrows this (see TODO #11).
		α Suspend()ι->void override{ ResumeExp( Exception{_sl, Jde::ELogLevel::Critical, "Start instance not implemented."} ); }
	};

	α AppInstanceHook::Start( const QL::MutationQL& m, UserPK userPK, SL sl )ι->up<TAwait<jvalue>>{
		//StartAwait( m, userPK, sl );
		return m.JsonTableName=="applicationInstance" ? mu<StartAwait>( m, userPK, sl ) : nullptr;
	}

	struct StopAwait : TAwait<jvalue>{
		StopAwait( QL::MutationQL mutation, UserPK userPK, sp<IApp> appClient, SL sl )ι:
			TAwait<jvalue>{ sl },
			_mutation{ mutation },
			_userPK{ userPK },
			_appClient{ move(appClient) }
		{}
		α Suspend()ι->void override{
			let id = _mutation.Id<ConnectionPK>();
			auto pid = id==_appClient->ConnectionPK() ? Process::ProcessId() : 0;
			if( auto p = pid ? sp<ServerSocketSession>{} : Server::FindConnection(id); p ){
				let& instance = p->Instance();
				//Process::Kill signals a pid in THIS host's namespace, but instance.pid() was measured on the instance's own
				//host - killing it here would hit whatever local process holds that number.  Only stop an instance that
				//advertises this AppServer's host (computed the same way the client advertises it, so it is symmetric).
				let thisHost = Settings::FindString("/http/host").value_or( Process::HostName() );
				if( instance.host()!=thisHost ){
					ResumeExp( Exception{_sl, {ELogTags::SocketServerRead, EHttpStatus::Unauthorized}, "Cannot stop instance on remote host '{}' (this host is '{}'); forward a stop over its socket instead.", instance.host(), thisHost} );
					return;
				}
				pid = instance.pid();
			}
			if( pid ){
				if( Process::Kill(pid) )
					ResumeScaler( 0 );
				else
					ResumeExp( Exception{_sl, {ELogTags::SocketServerRead}, "Failed to kill process."} );
			}
			else
				ResumeExp( Exception{_sl, {ELogTags::SocketServerRead}, "Instance not found."} );
		}
		QL::MutationQL _mutation;
		UserPK _userPK;
		sp<IApp> _appClient;
	};

	α AppInstanceHook::Stop( const QL::MutationQL& m, UserPK userPK, SL sl )ι->up<TAwait<jvalue>>{
		return m.JsonTableName=="applicationInstance" ? mu<StopAwait>( m, userPK, _appClient, sl ) : nullptr;
	}
}