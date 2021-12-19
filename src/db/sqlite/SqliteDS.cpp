#include "SqliteDS.h"

Jde::DB::IDataSource* GetDataSource()
{
	return new Jde::DB::Sqlite::SqliteDS();
}

namespace Jde::DB::Sqlite
{
}