#pragma once

namespace Jde::DB::Sqlite::Sql
{
	string ColumnSql( sv tablePrefix, bool addTable=false )ι;
	//string IndexSql( bool addTable=false )noexcept;
	//string ForeignKeySql( bool addSchema=false )noexcept;
	//string ProcSql( bool addSchema=false )noexcept;
	//constexpr sv CatalogSql = "select DB_NAME()"sv;
}