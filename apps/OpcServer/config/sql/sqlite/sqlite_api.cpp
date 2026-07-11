#include <jde/db/sqlite_api.h>
#include "opcProcs.h"

//Loaded by OpcServer when the datasource is sqlite - the driver dispatches Sql::IsProc calls to these twins.
void RegisterAppServerProcs(){
	using namespace Jde::DB::Sqlite::OpcProcs;
	RegisterOpcConstructorInsert();
	RegisterOpcNodeIdInsert();
	RegisterOpcNodeInsert();
	RegisterOpcObjectInsert();
	RegisterOpcObjectTypeInsert();
	RegisterOpcVariableInsert();
	RegisterOpcVariantInsert();
}
