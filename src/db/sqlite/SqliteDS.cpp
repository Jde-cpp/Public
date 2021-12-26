#include "SqliteDS.h"

Jde::DB::IDataSource* GetDataSource()
{
	return new Jde::DB::Sqlite::SqliteDS();
}

namespace Jde::DB::Sqlite
{
	α SqliteDS::SchemaProc()noexcept->sp<ISchemaProc>
	{
		return ms<SchemaProc( shared_from_this() );
	}
}