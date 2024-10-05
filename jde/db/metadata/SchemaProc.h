#pragma once
#ifndef SCHEMA_PROC_H
#define SCHEMA_PROC_H
#include "Schema.h"
#include "Table.h"

namespace Jde::DB{
	struct IDataSource; struct Cluster; struct ForeignKey; struct Index; struct Procedure;
	struct SchemaDdl;
	struct ISchemaProc{
		ISchemaProc( sp<IDataSource> pDataSource ):_pDataSource{pDataSource}{}
		Γ α SyncSchema( const Schema& config, SchemaDdl& db, const fs::path& relativeScriptPath )ε->void;
		β LoadTables( sv catalog={} )ε->flat_map<string,Table> = 0;
		β LoadProcs( sv catalog={} )ε->flat_map<string,Procedure> = 0;
		β ToType( sv typeName )ι->EType=0;
		β LoadIndexes( sv tableName={} )ε->vector<Index> = 0;
		β LoadForeignKeys( sv catalog={} )ε->flat_map<string,ForeignKey> = 0;
	protected:
		sp<IDataSource> _pDataSource;
	};
}
#endif