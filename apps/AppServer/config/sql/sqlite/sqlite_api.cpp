#include <jde/db/sqlite_api.h>
#include "appProcs.h"
#include "../../../../../libs/access/config/sql/sqlite/accessProcs.h"

//Exported symbol the driver calls with its registry when the datasource is sqlite - it dispatches Sql::IsProc
//calls to the twins registered here (AppServer hosts access, so both proc sets register through this DLL).
void RegisterProcs( Jde::DB::Sqlite::IProcs& procs ){
	using namespace Jde::DB::Sqlite;
	AppProcs::RegisterAppConnectionInsert( procs );
	AppProcs::RegisterAppInstanceInsert( procs );
	AppProcs::RegisterAppInstanceTagLevelUpsert( procs );
	AppProcs::RegisterAppProgramInsert( procs );

	AccessProcs::RegisterAccessAcInsertRole( procs );
	AccessProcs::RegisterAccessAcUpsertPermission( procs );
	AccessProcs::RegisterAccessProviderPurge( procs );
	AccessProcs::RegisterAccessRoleAdd( procs );
	AccessProcs::RegisterAccessRoleInsert( procs );
	AccessProcs::RegisterAccessRolePurge( procs );
	AccessProcs::RegisterAccessRoleRemove( procs );
	AccessProcs::RegisterAccessUserInsert( procs );
	AccessProcs::RegisterAccessUserInsertKey( procs );
	AccessProcs::RegisterAccessUserInsertLogin( procs );
}
