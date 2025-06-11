#include "globals.h"
#include <jde/db/meta/AppSchema.h>
#include "UAServer.h"

namespace Jde::Opc {
	sp<DB::AppSchema> _appSchema;
	string _serverName;
	Server::UAServer _ua;

	α Server::DataType( uint i )ε->const UA_DataType&{
		THROW_IF( i>= UA_TYPES_COUNT, "Invalid data type index: '{}'", i );
		return UA_TYPES[i];
	}
	α Server::DS()ι->DB::IDataSource&{ return *_appSchema->DS(); }
	α Server::GetView( str name )ε->const DB::View&{ return _appSchema->GetView(name); }
	α Server::GetViewPtr( str name )ε->sp<DB::View>{ return _appSchema->GetViewPtr(name); }
	α Server::ServerName()ι->str{ return _serverName; }
	α Server::SetServerName( str serverName )ι->void{ _serverName = serverName; }
	α Server::SetSchema( sp<DB::AppSchema> schema )ι->void{ _appSchema = schema; }
	α Server::GetUAServer()ι->UAServer&{ return _ua; }
}