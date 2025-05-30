#pragma once
#include <jde/db/meta/Table.h>

namespace Jde::DB{
	struct Index; struct SchemaDdl;

	struct TableDdl final: Table{
		TableDdl( const Table& table )ι:Table{table}{};

		α InsertProcCreateStatement( const Table& config )Ι->string;
		α CreateStatement()Ε->string;

		vector<Index> Indexes;
	};
}