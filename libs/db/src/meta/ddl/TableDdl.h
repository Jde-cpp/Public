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
}