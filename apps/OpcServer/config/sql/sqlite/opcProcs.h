#pragma once
#include "../../../../../libs/db/drivers/sqlite/src/SqliteProcs.h"

struct sqlite3;

//Native twins of the server procs in the sibling mysql dir - one <proc>.cpp per proc, same names,
//same param order, out params returned as the result row. RegisterAppServerProcs (jde/db/sqlite_api.h) registers them all.
namespace Jde::DB::Sqlite::OpcProcs{
	α RegisterOpcConstructorInsert()ι->void;
	α RegisterOpcNodeIdInsert()ι->void;
	α RegisterOpcNodeInsert()ι->void;
	α RegisterOpcObjectInsert()ι->void;
	α RegisterOpcObjectTypeInsert()ι->void;
	α RegisterOpcVariableInsert()ι->void;
	α RegisterOpcVariantInsert()ι->void;

	//Shared bodies - the object/variable/variant procs `call` opc_node_id_insert. node_id = ns<<32 | (number
	//or crc32 of string/uuid-string/bytes); mysql's CRC32/BIN_TO_UUID are computed in C++ (sqlite has neither).
	α NodeIdInsert( sqlite3& db, const Value& ns, const Value& number, const Value& string_, const Value& guid, const Value& bytes, const Value& namespaceUri, const Value& serverIndex, const Value& isGlobal, SL sl )ε->uint; //returns the computed node_id.
	α EnsureDataTypeNodeId( sqlite3& db, const Value& dataTypeId, SL sl )ε->void; //inserts a ns=0 node id for builtin data types (<=32750) if missing.
}
