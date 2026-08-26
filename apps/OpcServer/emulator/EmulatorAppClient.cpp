#include "EmulatorAppClient.h"
#include <jde/app/IApp.h>
#include <jde/ql/IQL.h>

namespace Jde::Opc::Emulator{
	static sp<App::Client::IAppClient> _appClient = ms<EmulatorAppClient>();
	α AppClient()ι->sp<App::Client::IAppClient>{ return _appClient; }

	struct EmulatorQL : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		EmulatorQL( QL::RequestQL&& q, Jde::UserPK executer, SL sl )ι:
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
	α EmulatorAppClient::ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, SL sl )ε->up<TAwait<jvalue>>{
		return mu<EmulatorQL>( move(q), executer, sl );
	}

	α EmulatorQL::await_resume()ε->jvalue{
		THROW_IF( !_q.IsQueries(), "Only queries are supported." );
		jvalue y;
		for( const auto& table : _q.Queries() ){
			THROW_IF( table.JsonName!="status", "Table {} not supported.", table.JsonName );
			y = App::IApp::Status();
		}
		return y;
	}
}
