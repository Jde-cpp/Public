#include <jde/access/client/accessClient.h>
#include "../accessInternal.h"
namespace Jde::Access{
	α Client::Configure( sp<DB::AppSchema> accessSchema, vector<sp<DB::AppSchema>>&& localSchemas, sp<QL::IQL> appQL, UserPK executer, sp<Authorize> authorizer, sp<AccessListener> listener )ε->ConfigureAwait{
		SetSchema( accessSchema );
		return ConfigureAwait{ appQL, move(localSchemas), authorizer, executer, listener };
	}
}
