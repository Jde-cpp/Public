#pragma once
#include "../../../src/meta/IServerMeta.h"
#include <jde/db/generators/Sql.h>
//#include "../../../Framework/source/db/types/Schema.h"
//#include "../../../Framework/source/db/types/Table.h"
//#include "../../../Framework/source/db/SchemaProc.h"

namespace Jde::DB{
	struct IDataSource;
namespace MsSql{
	struct MsSqlSchemaProc final : IServerMeta{
		MsSqlSchemaProc( sp<IDataSource> dataSource )ι:
			IServerMeta{ dataSource }
		{}
		α LoadTables( sv schemaName, sv tablePrefix )Ε->flat_map<string,sp<Table>> override;
		α LoadTable( str schemaName, str tableName, SL sl )Ε->sp<TableDdl> override;
		α ToType( sv typeName )Ι->EType override;
		α LoadIndexes( sv schema, sv tableName )Ε->vector<Index> override;
		α LoadForeignKeys( str schemaName )Ε->flat_map<string,ForeignKey> override;
		α LoadProcs( str schemaName )Ε->flat_map<string,Procedure> override;
	private:
		α LoadColumns( Sql&& sql )Ε->flat_map<string,sp<Table>>;
	};
}}