#pragma once
#include "../../src/meta/IServerMeta.h"

namespace Jde::DB{
	struct IDataSource; struct ForeignKey; struct Index; struct Procedure; struct Table;
}

namespace Jde::DB::MySql{
	struct MySqlServerMeta final : IServerMeta{
		MySqlServerMeta( sp<IDataSource> pDataSource ):
			IServerMeta{ pDataSource }
		{}
		α LoadTables( sv schemaName, sv tablePrefix )Ε->flat_map<string,sp<Table>> override;
		α LoadTable( str schemaName, str tableName, SRCE )Ε->sp<TableDdl> override;
		α ToType( sv name )Ι->EType override;
		α LoadIndexes( sv tablePrefix, sv tableName={} )Ε->vector<Index> override;
		α LoadForeignKeys( str schemaName )Ε->flat_map<string,ForeignKey> override;
		α LoadProcs( str schemaName )Ε->flat_map<string,Procedure> override;
	};
}