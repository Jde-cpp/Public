#pragma once
#include "../usings.h"

namespace Jde::DB{
	struct Cluster; struct ForeignKey; struct IDataSource;  struct Index; struct Procedure; struct AppSchema; struct SchemaDdl; struct Table;

	struct IServerMeta{
		IServerMeta( sp<IDataSource> p ):_pDataSource{p}{}
		//α Sync( const Schema& config, const jobject& jconfig )ε->void;

		β LoadIndexes( sv tableName={} )Ε->vector<Index> = 0;
		β LoadTables()Ε->flat_map<string,sp<Table>> = 0;
		β LoadForeignKeys()Ε->flat_map<string,ForeignKey> = 0;
		β LoadProcs()Ε->flat_map<string,Procedure> = 0;
	private:
		β ToType( sv typeName )Ι->EType=0;
	protected:
		sp<IDataSource> _pDataSource;
	};
}
