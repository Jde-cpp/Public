#pragma once
#include <jde/app/client/IAppClient.h>

namespace Jde::Opc::Emulator{
	α Run( sp<App::Client::IAppClient> client )ε->int;//blocks until shutdown or -duration; returns the process exit code.
	//createAcl granting this user AllAccess on the OpcServer's `nodeIds` resource.  The resource exists once the OpcServer
	//has booted, and OpcServer must be RESTARTED afterwards - it assigns node rights at startup (OpcAuthorize::AssignRights).
	α GrantWriteRights( const sp<App::Client::IAppClient>& client )ι->void;
	//Both certificates: the AppServer login cert (/http/ssl) and the UA channel cert (/emulator/ssl).  The latter's
	//directory must be one of the OpcServer's /access/trustedCertDirs (config/args/*/args.libsonnet names it).
	α CreateCertificates()ε->void;
}
