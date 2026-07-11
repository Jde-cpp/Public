#include <jde/db/sqlite_api.h>
#include "appProcs.h"
#include "../../../../../libs/access/config/sql/sqlite/accessProcs.h"

//Loaded by the app when the datasource is sqlite - the driver dispatches Sql::IsProc calls to these twins.
void RegisterAppServerProcs(){
	using namespace Jde::DB::Sqlite;
	AppProcs::RegisterAppConnectionInsert();
	AppProcs::RegisterAppInstanceInsert();
	AppProcs::RegisterAppInstanceTagLevelUpsert();
	AppProcs::RegisterAppProgramInsert();

	AccessProcs::RegisterAccessAcInsertRole();
	AccessProcs::RegisterAccessAcUpsertPermission();
	AccessProcs::RegisterAccessProviderPurge();
	AccessProcs::RegisterAccessRoleAdd();
	AccessProcs::RegisterAccessRoleInsert();
	AccessProcs::RegisterAccessRolePurge();
	AccessProcs::RegisterAccessRoleRemove();
	AccessProcs::RegisterAccessUserInsert();
	AccessProcs::RegisterAccessUserInsertKey();
	AccessProcs::RegisterAccessUserInsertLogin();
}
