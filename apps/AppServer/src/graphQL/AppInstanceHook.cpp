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
		α Suspend()ι->void override{}
		[[noreturn]] α await_resume()ε->jvalue override{
			throw Exception{ _sl, Jde::ELogLevel::Critical, "Start instance not implemented" };
		}
	};

	α AppInstanceHook::Start( const QL::MutationQL& m, UserPK userPK, SL sl )ι->up<TAwait<jvalue>>{
		//StartAwait( m, userPK, sl );
		return m.JsonTableName=="applicationInstance" ? mu<StartAwait>( m, userPK, sl ) : nullptr;
	}

	struct StopAwait : TAwait<jvalue>{
		StopAwait( QL::MutationQL mutation, UserPK userPK, sp<IApp> appClient, SL sl )ι:
			TAwait<jvalue>{ sl },
			_mutation{mutation},
			_userPK{userPK},
			_appClient{move(appClient)}
		{}
		α Suspend()ι->void override{
			let id = _mutation.Id<AppInstancePK>();
			auto pid = id==_appClient->InstancePK() ? Process::ProcessId() : 0;
			if( auto p = pid ? sp<ServerSocketSession>{} : Server::FindInstance( id ); p )
				pid = p->Instance().pid();
			if( pid ){
				if( !Process::Kill(pid) )
					ResumeScaler( 0 );
				else
					ResumeExp( Exception{ELogTags::SocketServerRead, _sl, "Failed to kill process."} );
			}
			else
				ResumeExp( Exception{ELogTags::SocketServerRead, _sl, "Instance not found."} );
		}
		QL::MutationQL _mutation;
		UserPK _userPK;
		sp<IApp> _appClient;
	};

	α AppInstanceHook::Stop( const QL::MutationQL& m, UserPK userPK, SL sl )ι->up<TAwait<jvalue>>{
		return m.JsonTableName=="applicationInstance" ? mu<StopAwait>( m, userPK, _appClient, sl ) : nullptr;
	}
}