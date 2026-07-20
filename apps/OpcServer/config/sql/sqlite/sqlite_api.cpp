#include <jde/db/sqlite_api.h>
#include "opcProcs.h"

//Exported symbol the driver calls with its registry when OpcServer's datasource is sqlite - it dispatches
//Sql::IsProc calls to the twins registered here.
void RegisterProcs( Jde::DB::Sqlite::IProcs& procs ){
	using namespace Jde::DB::Sqlite::OpcProcs;
	RegisterOpcBrowseNameInsert( procs );
	RegisterOpcConstructorInsert( procs );
	RegisterOpcNodeIdInsert( procs );
	RegisterOpcNodeInsert( procs );
	RegisterOpcObjectInsert( procs );
	RegisterOpcObjectTypeInsert( procs );
	RegisterOpcServerInsert( procs );
	RegisterOpcVariableInsert( procs );
	RegisterOpcVariantInsert( procs );
}
