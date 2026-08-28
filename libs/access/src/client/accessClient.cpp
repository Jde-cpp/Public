#include <jde/access/client/accessClient.h>
#include "../accessInternal.h"
namespace Jde::Access{
	struct ConfigureContext final{ vector<sp<DB::AppSchema>> Schemas; sp<Authorize> Authorizer; UserPK Executer; sp<AccessListener> Listener; string ResourceSchema; };
	static optional<ConfigureContext> _configured;

	α Client::Configure( sp<DB::AppSchema> accessSchema, vector<sp<DB::AppSchema>>&& localSchemas, sp<QL::IQL> appQL, UserPK executer, sp<Authorize> authorizer, sp<AccessListener> listener, string resourceSchema, SL sl )ε->ConfigureAwait{
		SetSchema( accessSchema );
		_configured = ConfigureContext{ localSchemas, authorizer, executer, listener, resourceSchema };
		return ConfigureAwait{ appQL, move(localSchemas), authorizer, executer, listener, resourceSchema, false, false, sl };//a client follows only its own schemas.
	}
	α Client::IsConfigured()ι->bool{ return _configured.has_value(); }
	α Client::Reload( sp<QL::IQL> appQL, SL sl )ε->ConfigureAwait{
		THROW_IFSL( !_configured, "Access::Client::Reload before Configure." );
		const auto& c = *_configured;
		return ConfigureAwait{ move(appQL), c.Schemas, c.Authorizer, c.Executer, c.Listener, c.ResourceSchema, true, false, sl };
	}
}
