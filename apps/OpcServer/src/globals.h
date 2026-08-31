#pragma once
namespace Jde::DB{ struct AppSchema; }

namespace Jde::App::Client{ struct IAppClient; }
namespace Jde::Opc::Server {
	struct UAServer; struct PubSubReader;

	α Initialize( sp<DB::AppSchema> schema )ε->void;
	α AppClient()ι->sp<App::Client::IAppClient>;
	α GetSchema()ι->DB::AppSchema&;
	α GetSchemaPtr()ι->sp<DB::AppSchema>;
	α GetUAServer()ι->UAServer&;
	α FindUAServer()ι->UAServer*;//null before Initialize and after shutdown - Access::Client::Configure installs the acl listener before either.
	//the Part 14 subscriber on the current UAServer (/opcServer/pubsub); replaces any earlier one.
	α StartPubSub( const jobject& settings, SRCE )ε->void;
	α PubSub()ι->PubSubReader*;//null until StartPubSub.
}