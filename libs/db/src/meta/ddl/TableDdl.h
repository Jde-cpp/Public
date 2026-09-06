#pragma once
#include <jde/db/meta/Table.h>
#include "Index.h"

namespace Jde::DB{
	struct Index; struct SchemaDdl;

	struct ΓDB TableDdl final: Table{
		TableDdl( const Table& table )ι:Table{table}{};
		~TableDdl()override;//out-of-line key function: anchors typeinfo/vtable to libJde.DB.so so dynamic_cast<TableDdl> works across the driver .so boundary.

		α InsertProcCreateStatement( const Table& config )Ι->string;
		α CreateStatement()Ε->string;

		vector<Index> Indexes;
	};

	//An insert proc's name without its schema.  InsertProcName derives from DBName, which carries a `schema.` when the
	//app schema is not the connection's default (AppSchema::Initialize), and the DDL re-qualifies the bare name itself
	//(db-refactor A10: SyncTables and InsertProcCreateStatement each stripped it in place).
	Ξ UnqualifiedProcName( string procName )ι->string{
		if( const auto index = procName.find_first_of('.'); index<procName.size()-1 )
			procName = procName.substr( index+1 );
		return procName;
	}
}