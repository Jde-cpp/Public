#include <jde/ql/ql.h>
#include <jde/fwk/settings.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLAwait.h>
#include <jde/ql/LocalSubscriptions.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/Introspection.h>
#include <jde/ql/LocalQL.h>

#define let const auto

namespace Jde::Access{ struct Authorize; }
namespace Jde{
	α QL::Configure( vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer )ε->sp<LocalQL>{
		for( let& schema : schemas ){
			if( let path = Settings::FindSV(schema->ConfigPath()+"/ql"); path )
				AddIntrospection( {Json::ReadJsonNet(*path)} );
		}
		return ms<LocalQL>( move(schemas), authorizer );
	}
}