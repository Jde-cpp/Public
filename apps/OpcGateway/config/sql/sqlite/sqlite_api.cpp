#include <jde/db/sqlite_api.h>
#include "gatewayProcs.h"

//Loaded by OpcGateway when the datasource is sqlite - the driver dispatches Sql::IsProc calls to the twins.
void RegisterAppServerProcs(){
	Jde::DB::Sqlite::GatewayProcs::RegisterGatewayServerConnectionInsert();
}
