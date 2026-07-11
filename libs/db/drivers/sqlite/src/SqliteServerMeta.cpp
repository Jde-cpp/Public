#include <sqlite3.h>
#include "SqliteServerMeta.h"
#include <jde/db/IDataSource.h>
#include <jde/db/meta/Table.h>
#include "../../src/meta/ddl/TableDdl.h"
#include "SqliteProcs.h"

#define let const auto

namespace Jde::DB::Sqlite{
	//TODO flesh out - mirror MySqlServerMeta.cpp but query pragmas instead of information_schema.
	α SqliteServerMeta::LoadTables( sv /*schemaName*/, sv /*tablePrefix*/ )Ε->flat_map<string,sp<Table>>{
		//select name from sqlite_master where type='table' and name like '<tablePrefix>%', then LoadTable each.
		THROW( "SqliteServerMeta::LoadTables not implemented." );
	}
	α SqliteServerMeta::LoadTable( str /*schemaName*/, str /*tableName*/, SL )Ε->sp<TableDdl>{
		//pragma table_info('<tableName>') -> cid,name,type,notnull,dflt_value,pk.
		THROW( "SqliteServerMeta::LoadTable not implemented." );
	}
	α SqliteServerMeta::ToType( sv /*name*/ )Ι->EType{
		//sqlite type affinity - declared types survive in sqlite_master/table_info, so 'int unsigned',
		//'varchar(255)' etc. from TableDdl round-trip; map by affinity rules as the fallback.
		return EType::None; //TODO
	}
	α SqliteServerMeta::LoadIndexes( sv /*tablePrefix*/, sv /*tableName*/ )Ε->vector<Index>{
		//pragma index_list('<table>') + pragma index_info('<index>').
		THROW( "SqliteServerMeta::LoadIndexes not implemented." );
	}
	α SqliteServerMeta::LoadForeignKeys( str /*schemaName*/ )Ε->flat_map<string,ForeignKey>{
		//pragma foreign_key_list('<table>') per table.
		THROW( "SqliteServerMeta::LoadForeignKeys not implemented." );
	}
	α SqliteServerMeta::LoadProcs( str /*schemaName*/ )Ε->flat_map<string,Procedure>{
		//No server procs - report the native registry so DDL sync treats registered procs as existing.
		THROW( "SqliteServerMeta::LoadProcs not implemented." );
	}
}
