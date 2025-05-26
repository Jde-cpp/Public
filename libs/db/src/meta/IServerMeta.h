#pragma once
#include "../usings.h"

namespace Jde::DB{
	struct Cluster; struct ForeignKey; struct IDataSource;  struct Index; struct Procedure; struct AppSchema; struct SchemaDdl; struct Table;

	struct IServerMeta{
		IServerMeta( sp<IDataSource> p ):_pDataSource{p}{}

		β LoadTables( sv schemaName, sv tablePrefix )Ε->flat_map<string,sp<Table>> = 0;
		β LoadIndexes( sv tablePrefix, sv tableName={} )Ε->vector<Index> = 0;
		β LoadForeignKeys( str schemaName )Ε->flat_map<string,ForeignKey> = 0;
		β LoadProcs( str schemaName )Ε->flat_map<string,Procedure> = 0;
	private:
		β ToType( sv typeName )Ι->EType=0;
	protected:
		sp<IDataSource> _pDataSource;
	};
}