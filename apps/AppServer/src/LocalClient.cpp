#include "LocalClient.h"
#include <jde/app/log/ProtoLog.h>

namespace Jde::App{
	sp<Server::LocalClient> _appClient = ms<Server::LocalClient>();
	α Server::AppClient()ι->sp<LocalClient>{ return _appClient; }
namespace Server{
	α LocalClient::InitLogging()ι->void{
		Logging::Add<ProtoLog>( "proto" );
		Logging::Init();
	}

	struct AppServerQL : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		AppServerQL( QL::RequestQL&& q, Jde::UserPK executer, bool raw, SL sl )ι:
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
	α LocalClient::ClientQuery( QL::RequestQL&& q, UserPK executer, bool raw, SL sl )ε->up<TAwait<jvalue>>{
		return mu<AppServerQL>( move(q), executer, raw, sl );
	}

	α AppServerQL::await_resume()ε->jvalue{
		THROW_IF( !_q.IsQueries(), "Only queries are supported." );
		jvalue y;
		for( const auto& table : _q.Queries() ){
			THROW_IF( table.JsonName!="status", "Table {} not supported.", table.JsonName );
			y = App::IApp::Status();
		}
		return y;
	}
}}