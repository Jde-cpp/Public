#include "ClientQLAwait.h"
#include <jde/ql/ql.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/appClient.h>

namespace Jde::App::Client{
	struct QLAwait final : TAwait<jvalue>{
		QLAwait( string query, SRCE )ε:_query{move(query)}{}
		α Suspend()ι->void override{ Execute(); }
	private:
		α Execute()ι->Web::Client::ClientSocketAwait<string>::Task;
		string _query;
	};
	struct ClientQLServer final : QL::IQL{
		α Query( string query, UserPK _, SRCE )ι->up<TAwait<jvalue>> override{ return mu<QLAwait>( query, sl ); }
	};

	α QLAwait::Execute()ι->Web::Client::ClientSocketAwait<string>::Task{
		try{
			auto result = co_await GraphQL( _query, _sl );
			Resume( Json::ParseValue(move(result)) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}
namespace Jde::App{
	α Client::QLServer()ι->sp<QL::IQL>{ return ms<ClientQLServer>(); }
}
