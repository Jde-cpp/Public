#include "OpcServerAppClient.h"
#include <jde/app/IApp.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/app/log/LogSettingsAwait.h>
#include <jde/ql/IQL.h>
#include <jde/ql/QLAwait.h>
#include "ql/OpcQL.h"

namespace Jde::Opc::Server{
	struct OpcServerQL : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		OpcServerQL( QL::RequestQL&& q, Jde::UserPK executer, SL sl )ι:
			base{ sl },
			_q{ move(q) },
			_executer{ executer }
		{}
		α await_ready()ι->bool override{ return true; }
		α await_resume()ε->jvalue override;
	private:
		α Suspend()ι->void override{ ASSERT(false); }
		QL::RequestQL _q;
		Jde::UserPK _executer;
	};
	//The app server pushes log-level changes as updateLogSetting mutations; OpcServerQL below answers `status` and nothing
	//else, so without this the push came back "Only queries are supported" and the levels never moved.  Only log settings
	//get through to the ql - everything else stays as restricted as it was.
	Ω isLogSettings( const QL::RequestQL& q )ι->bool{
		return q.IsMutation() && std::ranges::all_of( q.Mutations(), [](const auto& m){return App::LogSettingsMAwait::IsApplicable(m);} );
	}
	α OpcServerAppClient::ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, SL sl )ε->up<TAwait<jvalue>>{
		return isLogSettings( q )
			? mu<QL::QLAwait<>>( move(q), QL::Creds{executer}, Server::QLPtr(), sl )
			: up<TAwait<jvalue>>{ mu<OpcServerQL>(move(q), executer, sl) };
	}

	α OpcServerQL::await_resume()ε->jvalue{
		THROW_IF( !_q.IsQueries(), "Only queries are supported." );
		jvalue y;
		for( const auto& table : _q.Queries() ){
			THROW_IF( table.JsonName!="status", "Table {} not supported.", table.JsonName );
			y = App::IApp::Status();
		}
		return y;
	}
}