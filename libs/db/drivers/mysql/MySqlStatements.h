#pragma once


namespace Jde::DB::MySql::Ddl{
	α ColumnSql( sv tablePrefix, bool addTable=false )ι->string;
	α ForeignKeySql( bool addSchema=false )ι->string;
	α IndexSql( sv tablePrefix, bool addTable=false )ι->string;
	α ProcSql( bool addSchema=false )ι->string;
}