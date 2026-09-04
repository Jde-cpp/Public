#include <jde/access/client/accessClient.h>
#include "../accessInternal.h"
namespace Jde::Access{
	α Client::Configure( sp<DB::AppSchema> accessSchema, const Client::Context& c, sp<QL::IQL> appQL, bool syncOnly, SL sl )ε->ConfigureAwait{
		SetSchema( accessSchema );//the library's own access-schema pointer - still process-wide, and fine: every client in a process passes the one cached "access" schema.
		return ConfigureAwait{ move(appQL), c.Schemas, c.Authorizer, c.Executer, c.Listener, c.ResourceSchema, false, false, syncOnly, sl };//a client follows only its own schemas.
	}
	α Client::Reload( const Client::Context& c, sp<QL::IQL> appQL, SL sl )ε->ConfigureAwait{
		return ConfigureAwait{ move(appQL), c.Schemas, c.Authorizer, c.Executer, c.Listener, c.ResourceSchema, true, false, false, sl };
	}
}
