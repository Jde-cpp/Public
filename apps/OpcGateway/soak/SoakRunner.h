#pragma once
#include <jde/app/client/IAppClient.h>

namespace Jde::Opc::Gateway::Soak{
	α Run( sp<App::Client::IAppClient> client )ε->int;//blocks for the configured duration; returns the process exit code.
	//createAcl granting this user AllAccess on the opc nodeIds resource. The resource must already exist (OpcServer's
	//first boot registers it) and OpcServer must be RESTARTED afterwards to load the acl - live acl events reach a
	//split-process OpcServer but never update its in-memory rights (see soak findings).
	α GrantWriteRights( const sp<App::Client::IAppClient>& client )ι->void;
}
