#pragma once
namespace Jde::DB{ struct IDataSource; struct AppSchema; struct View; }

namespace Jde::Opc::Server {
	struct UAServer;

	α DS()ι->DB::IDataSource&;
	α GetView( str name )ε->const DB::View&;
	α GetViewPtr( str name )ε->sp<DB::View>;
	α ServerId()->uint32;
	α SetServerId( uint32 id )ι->void;
	α GetSchema()ι->DB::AppSchema&;
	α SetSchema( sp<DB::AppSchema> schema )ι->void;
	α GetUAServer()ι->UAServer&;
}