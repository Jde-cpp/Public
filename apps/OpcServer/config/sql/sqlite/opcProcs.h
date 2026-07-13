#pragma once
#include <jde/db/sqlite_api.h>

struct sqlite3;

//Native twins of the server procs in the sibling mysql dir - one <proc>.cpp per proc, same names, same param
//order, out params returned as the result row. RegisterProcs (jde/db/sqlite_api.h) registers them all through
//the driver's IProcs, so this DLL needn't link the driver.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcConstructorInsert( IProcs& procs )ι->void;
	α RegisterOpcNodeIdInsert( IProcs& procs )ι->void;
	α RegisterOpcNodeInsert( IProcs& procs )ι->void;
	α RegisterOpcObjectInsert( IProcs& procs )ι->void;
	α RegisterOpcObjectTypeInsert( IProcs& procs )ι->void;
	α RegisterOpcVariableInsert( IProcs& procs )ι->void;
	α RegisterOpcVariantInsert( IProcs& procs )ι->void;

	//Shared bodies - the object/variable/variant procs `call` opc_node_id_insert. node_id = ns<<32 | (number
	//or crc32 of string/uuid-string/bytes); mysql's CRC32/BIN_TO_UUID are computed in C++ (sqlite has neither).
	α NodeIdInsert( IProcs& procs, sqlite3& db, const Value& ns, const Value& number, const Value& string_, const Value& guid, const Value& bytes, const Value& namespaceUri, const Value& serverIndex, const Value& isGlobal, SL sl )ε->uint; //returns the computed node_id.
	α EnsureDataTypeNodeId( IProcs& procs, sqlite3& db, const Value& dataTypeId, SL sl )ε->void; //inserts a ns=0 node id for builtin data types (<=32750) if missing.
}
