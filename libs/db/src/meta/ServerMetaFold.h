#pragma once
#include "ddl/ForeignKey.h"
#include "ddl/Index.h"

//C13: the row folds MySqlServerMeta and MsSqlSchemaProc both run over an INFORMATION_SCHEMA-style result set - one row
//per constraint *column*, to be gathered into one entry per constraint.  They were byte-identical, and db-review2 #1 had
//to patch the same fold in both.  Row parsing stays in the callers: the two dialects read `unique` differently (GetInt vs
//GetBit) and compute primaryKey differently (`PRIMARY` vs `<table>_pk`).
//
//NOT for the sqlite loaders.  They read pragma_foreign_key_list, which is already one row per column and *may legitimately
//repeat a column* - `foreign key(a,a) references p(x,y)` yields `a` twice, and the dedupe below would collapse it to one.
namespace Jde::DB{
	Ξ FoldForeignKeyRow( flat_map<string,ForeignKey>& fks, str name, str fkTable, str column, str pkTable )ι->void{
		auto pExisting = fks.find( name );
		if( pExisting==fks.end() )
			fks.emplace( name, ForeignKey{name, fkTable, {column}, pkTable} );
		else if( auto& columns=pExisting->second.Columns; find(columns, column)==columns.end() )
			columns.push_back( column );//a constraint can't list the same column twice, so a repeat is a duplicated row - folding it in would break SyncFKs' Columns compare and re-create every fk each sync.
	}

	Ξ FoldIndexRow( vector<Index>& indexes, str tableName, str indexName, str columnName, bool unique, bool primaryKey )ι->void{
		auto pExisting = find_if( indexes, [&](const auto& index){ return index.Name==indexName && index.TableName==tableName; } );
		auto& columns = pExisting==indexes.end()
			? indexes.emplace_back( indexName, tableName, primaryKey, nullptr, unique, false ).Columns //clustered: both loaders hard-coded false.
			: pExisting->Columns;
		columns.push_back( columnName );
	}
}
