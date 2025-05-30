#pragma once

namespace Jde::DB::MsSql::Sql{
	string ColumnSql( bool addTable=false )ι;
	string IndexSql( bool addTable=false )ι;
	string ForeignKeySql( bool addSchema=false )ι;
	string ProcSql( bool addSchema=false )ι;
	constexpr sv CatalogSql = "select DB_NAME()"sv;
}