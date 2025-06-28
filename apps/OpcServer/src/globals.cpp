#include "globals.h"
#include <jde/db/meta/AppSchema.h>
#include "UAServer.h"

namespace Jde::Opc {
	sp<DB::AppSchema> _appSchema;
	uint32 _serverId{};
	Server::UAServer _ua;

	α Server::DS()ι->DB::IDataSource&{ return *_appSchema->DS(); }
	α Server::GetView( str name )ε->const DB::View&{ return _appSchema->GetView(name); }
	α Server::GetViewPtr( str name )ε->sp<DB::View>{ return _appSchema->GetViewPtr(name); }
	α Server::ServerId()->uint32{ ASSERT(_serverId) return _serverId; }
	α Server::SetServerId( uint32 id )ι->void{ _serverId = id; }

	α Server::GetSchema()ι->DB::AppSchema&{ return *_appSchema; }
	α Server::SetSchema( sp<DB::AppSchema> schema )ι->void{ _appSchema = schema; }
	α Server::GetUAServer()ι->UAServer&{ return _ua; }
}