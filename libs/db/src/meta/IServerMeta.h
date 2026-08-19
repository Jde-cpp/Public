#pragma once
#include "../usings.h"

namespace Jde::DB{
	struct Cluster; struct ForeignKey; struct IDataSource;  struct Index; struct Procedure; struct AppSchema; struct SchemaDdl; struct Table; struct TableDdl;

	struct IServerMeta{
		//#52: a reference, not an sp.  The meta is a up<> member of the data source, so it strictly cannot outlive it -
		//and holding an sp made every data source that reached ServerMeta() own itself, so its destructor never ran and
		//sqlite never got its close/WAL-checkpoint (nor MySQL its session close).  A reference also cannot be rebound or
		//nulled, which is the invariant: this meta belongs to that data source for its whole life.
		IServerMeta( IDataSource& ds ):_ds{ds}{}

		β LoadTables( sv schemaName, sv tablePrefix )Ε->flat_map<string,sp<Table>> = 0;
		β LoadTable( str schemaName, str tableName, SRCE )Ε->sp<TableDdl> = 0;
		β LoadIndexes( sv tablePrefix, sv tableName={} )Ε->vector<Index> = 0;
		β LoadForeignKeys( str schemaName )Ε->flat_map<string,ForeignKey> = 0;
		β LoadProcs( str schemaName )Ε->flat_map<string,Procedure> = 0;
	private:
		β ToType( sv typeName )Ι->EType=0;
	protected:
		IDataSource& _ds;
	};
}