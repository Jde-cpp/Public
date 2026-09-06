#pragma once

//The INFORMATION_SCHEMA text the mysql and sql-server loaders share (db-refactor A13).  Each dialect keeps its own
//column and index listings - mysql reads INFORMATION_SCHEMA.COLUMNS/STATISTICS, sql server sys.columns/sys.indexes -
//but the fk and proc listings are the standard views, so the statement lives here and the dialect supplies only what
//differs: how the two KEY_COLUMN_USAGE sides are joined, and the where.
namespace Jde::DB::InformationSchema{
	//One row per fk *column*, ordered so FoldForeignKeyRow sees a constraint's columns in key order.  Both joins must
	//carry the schema - constraint names are only unique per schema, and joining on name alone fanned each row out
	//across every same-named constraint on the server (db-review2 #1); how the referenced side is matched is the
	//dialect's (mysql needs a COLLATE and matches REFERENCED_TABLE_NAME, sql server matches TABLE_SCHEMA).
	Ξ ForeignKeySql( sv fkJoin, sv pkJoin, sv where )ι->string{
		return Ƒ( "select fk.CONSTRAINT_NAME name, fk.TABLE_NAME foreign_table, fk.COLUMN_NAME fk, pk.TABLE_NAME primary_table, pk.COLUMN_NAME pk, pk.ORDINAL_POSITION ordinal\n"
			"from INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS con\n"
			"\tjoin INFORMATION_SCHEMA.KEY_COLUMN_USAGE fk on {}\n"
			"\tjoin INFORMATION_SCHEMA.KEY_COLUMN_USAGE pk on {}\n"
			"{}order by name, ordinal", fkJoin, pkJoin, where );
	}
	Ξ ProcSql( bool addSchema )ι->string{
		return Ƒ( "select SPECIFIC_NAME from INFORMATION_SCHEMA.ROUTINES{}", addSchema ? " where ROUTINE_SCHEMA=?" : "" );
	}
}