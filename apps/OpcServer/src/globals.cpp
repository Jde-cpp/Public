#include "globals.h"
#include <jde/db/meta/AppSchema.h>
#include <jde/app/client/IAppClient.h>
#include "OpcServerAppClient.h"
#include "UAServer.h"

namespace Jde::Opc {
	sp<DB::AppSchema> _appSchema;
	uint32 _serverId{};
	up<Server::UAServer> _ua;
	static sp<App::Client::IAppClient> _appClient = ms<Server::OpcServerAppClient>();

	α Server::Initialize( uint32 serverId, sp<DB::AppSchema> schema )ι->void{
		_serverId = serverId;
		_appSchema = schema;
		_ua = mu<UAServer>();
	}

	α Server::DS()ι->DB::IDataSource&{ return *_appSchema->DS(); }
	α Server::GetView( str name )ε->const DB::View&{ return _appSchema->GetView(name); }
	α Server::GetViewPtr( str name )ε->sp<DB::View>{ return _appSchema->GetViewPtr(name); }
	α Server::ServerId()->uint32{ ASSERT(_serverId) return _serverId; }
	α Server::AppClient()ι->sp<App::Client::IAppClient>{ return _appClient; }

	α Server::GetSchema()ι->DB::AppSchema&{ return *_appSchema; }
	α Server::GetUAServer()ι->UAServer&{ return *_ua; }
}