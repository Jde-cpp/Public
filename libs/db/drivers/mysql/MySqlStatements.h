#pragma once


namespace Jde::DB::MySql::Sql{
	α ColumnSql( bool addTable=false )ι->string;
	α ForeignKeySql( bool addSchema=false )ι->string;
	α IndexSql( bool addTable=false )ι->string;
	α ProcSql( bool addSchema=false )ι->string;
}