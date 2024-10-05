#pragma once
#include <jde/db/metadata/Table.h>

namespace Jde::DB{
	struct SchemaDdl;

	struct TableDdl final: Table{
		TableDdl( const Table& table )ι:Table{table}{};

		α InsertProcText()Ι->string;
		α CreateText()Ι->string;

		sp<DB::SchemaDdl> Schema;
		vector<Index> Indexes;
	};
}