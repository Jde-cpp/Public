#pragma once
#include "../../meta/IServerMeta.h"

namespace Jde::DB{
	struct IDataSource; struct ForeignKey; struct Index; struct Procedure; struct Table;
}

namespace Jde::DB::MySql{
	struct MySqlServerMeta final : IServerMeta{
		MySqlServerMeta( sp<IDataSource> pDataSource ):
			IServerMeta{ pDataSource }
		{}
		α LoadTables()Ε->flat_map<string,sp<Table>> override;
		α ToType( sv name )Ι->EType override;
		α LoadIndexes( sv tableName={} )Ε->vector<Index> override;
		α LoadForeignKeys( )Ε->flat_map<string,ForeignKey> override;
		α LoadProcs()Ε->flat_map<string,Procedure> override;
	};
}