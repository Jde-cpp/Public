#pragma once
namespace Jde::DB{ struct IDataSource; struct AppSchema; struct View; }

namespace Jde::Opc::Server {
	struct UAServer;

	α Initialize( uint32 serverId, sp<DB::AppSchema> schema )ι->void;
	α DS()ι->DB::IDataSource&;
	α GetView( str name )ε->const DB::View&;
	α GetViewPtr( str name )ε->sp<DB::View>;
	α ServerId()->uint32;
	α GetSchema()ι->DB::AppSchema&;
	α GetUAServer()ι->UAServer&;
}