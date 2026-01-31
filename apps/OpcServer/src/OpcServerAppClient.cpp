#include "OpcServerAppClient.h"
#include <jde/app/IApp.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/awaits/SocketAwait.h>
#include <jde/ql/IQL.h>

namespace Jde::Opc::Server{
	struct OpcServerQL : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		OpcServerQL( QL::RequestQL&& q, Jde::UserPK executer, bool raw, SL sl )ι:
			base{ sl },
			_q{ move(q) },
			_executer{ executer },
			_returnRaw{ raw }
		{}
		α await_ready()ι->bool override{ return true; }
		α await_resume()ε->jvalue override;
	private:
		α Suspend()ι->void override{ ASSERT(false); }
		QL::RequestQL _q;
		Jde::UserPK _executer;
		bool _returnRaw;
	};
	α OpcServerAppClient::ClientQuery( QL::RequestQL&& q, Jde::UserPK executer, bool raw, SL sl )ε->up<TAwait<jvalue>>{
		return mu<OpcServerQL>( move(q), executer, raw, sl );
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