#include "globals.h"
#include "jde/fwk/process/process.h"
#include <jde/db/meta/AppSchema.h>
#include <jde/app/client/IAppClient.h>
#include "OpcServerAppClient.h"
#include "UAServer.h"
#include "pubsub/PubSubReader.h"

namespace Jde::Opc {
	sp<DB::AppSchema> _appSchema;
	uint32 _serverId{};
	up<Server::UAServer> _ua;
	up<Server::PubSubReader> _pubSub;//after _ua: destroyed first, its connection belongs to that server.
	static sp<App::Client::IAppClient> _appClient = ms<Server::OpcServerAppClient>();

	α Server::Initialize( uint32 serverId, sp<DB::AppSchema> schema )ε->void{
		_serverId = serverId;
		_appSchema = schema;
		_pubSub.reset();//a re-Initialize (test fixtures) replaces the server it was built on.
		_ua = mu<UAServer>();//throws
		Process::AddShutdownFunction( [](bool , SL){
			_pubSub.reset();
			_ua.reset();
		} );
	}

	α Server::DS()ι->DB::IDataSource&{ return *_appSchema->DS(); }
	α Server::GetView( str name )ε->const DB::View&{ return _appSchema->GetView(name); }
	α Server::GetViewPtr( str name )ε->sp<DB::View>{ return _appSchema->GetViewPtr(name); }
	α Server::ServerId()->uint32{ ASSERT(_serverId) return _serverId; }
	α Server::AppClient()ι->sp<App::Client::IAppClient>{ return _appClient; }

	α Server::GetSchema()ι->DB::AppSchema&{ return *_appSchema; }
	α Server::GetSchemaPtr()ι->sp<DB::AppSchema>{ return _appSchema; }
	α Server::GetUAServer()ι->UAServer&{ return *_ua; }
	α Server::StartPubSub( const jobject& settings, SL sl )ε->void{ _pubSub = mu<PubSubReader>( GetUAServer(), settings, sl ); }
	α Server::PubSub()ι->PubSubReader*{ return _pubSub.get(); }
}