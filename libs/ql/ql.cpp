#include <jde/ql/ql.h>
#include <jde/framework/settings.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLAwait.h>
#include <jde/ql/LocalSubscriptions.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/Introspection.h>
#include "LocalQL.h"

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };
	vector<sp<DB::AppSchema>> _schemas;
	α SetIntrospection( Introspection&& x )ι->void;
}
namespace Jde{
	α QL::Local()ι->sp<IQL>{ return ms<LocalQL>(); }
	α QL::Configure( vector<sp<DB::AppSchema>>&& schemas )ε->void{
		_schemas = move( schemas );
		for( let& schema : _schemas ){
			if( let path = Settings::FindSV(schema->ConfigPath()+"/ql"); path ){
				SetIntrospection( {Json::ReadJsonNet(*path)} );
			}
		}
	}
}

namespace Jde::QL{
	α GetTable( str tableName, SL sl )ε->sp<DB::View>{
		for( let& schema : _schemas ){
			if( let pTable = schema->FindView(tableName); pTable )
				return pTable;
		}
		//table was prefixed with schema name.
		for( let& schema : _schemas ){
			if( tableName.starts_with(schema->Name) && tableName.size()>schema->Name.size() )
				return GetTable( tableName.substr(schema->Name.size()+1), sl );
		}
		throw Exception{ sl, "Could not find table '{}'", tableName };
	}
  α Schemas()ι->const vector<sp<DB::AppSchema>>&{
    return _schemas;
  }
}