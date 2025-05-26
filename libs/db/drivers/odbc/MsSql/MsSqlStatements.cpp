#include "MsSqlStatements.h"

namespace Jde::DB::MsSql::Sql
{
	using std::endl;
	α ColumnSql( bool addTable )ι->string{
		std::ostringstream os;
		os << "select tables.name table_name, columns.name column_name, columns.column_id, constraints.definition, columns.is_nullable, types.name, columns.max_length, is_identity, case when indexes.object_id is null then 0 else 1 end, columns.precision, columns.scale" << endl
			<< "from sys.schemas" << endl
			<< "join sys.tables on tables.schema_id=schemas.schema_id" << endl
			<< "join sys.columns on tables.object_id=columns.object_id" << endl
			<< "join sys.types on columns.system_type_id=types.system_type_id" << endl
			<< "left join sys.indexes on tables.object_id=indexes.object_id and is_primary_key=1" << endl
			<< "left join sys.default_constraints constraints on columns.default_object_id=constraints.object_id" << endl
			<< "where schemas.name=?" << endl;
		if( addTable )
			os << "\tand tables.name like ?" << endl;
		os << "order by tables.name, columns.column_id" << endl;
		return os.str();
	}

	α IndexSql( bool addTable )ι->string{
		std::ostringstream os;
		os << "select tables.name table_name, indexes.name index_name, columns.name column_name, case when indexes.is_unique=0 then CAST(1 AS BIT) else CAST(0 AS BIT) end non_unique, index_columns.key_ordinal" << endl
			<< "from sys.indexes" << endl
			<< "\tjoin sys.index_columns ON  indexes.object_id = index_columns.object_id and indexes.index_id = index_columns.index_id" << endl
			<< "\tjoin sys.columns ON index_columns.object_id = columns.object_id and index_columns.column_id = columns.column_id" << endl
			<< "\tjoin sys.tables ON indexes.object_id = tables.object_id" << endl
			<< "\tjoin sys.schemas on tables.schema_id=schemas.schema_id" << endl
			<< "WHERE tables.is_ms_shipped = 0" << endl

			<< "\t\tand schemas.name=?" << endl;
		if( addTable )
			os << " and tables.name=?" << endl;
		os << "ORDER BY tables.name, indexes.name, index_columns.key_ordinal" << endl;
		return os.str();
	}

	α ForeignKeySql( bool addSchema )ι->string{
		std::ostringstream os;
		os << "select  fk.CONSTRAINT_NAME name, fk.TABLE_NAME foreign_table, fk.COLUMN_NAME fk, pk.TABLE_NAME primary_table, pk.COLUMN_NAME pk, pk.ORDINAL_POSITION ordinal" << endl
			<< "from INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS con" << endl
			<< "\tjoin INFORMATION_SCHEMA.KEY_COLUMN_USAGE fk on con.CONSTRAINT_NAME=fk.CONSTRAINT_NAME" << endl
			<< "\tjoin INFORMATION_SCHEMA.KEY_COLUMN_USAGE pk on pk.CONSTRAINT_NAME=con.UNIQUE_CONSTRAINT_NAME and pk.ORDINAL_POSITION=fk.ORDINAL_POSITION --and pk.TABLE_NAME=con.REFERENCED_TABLE_NAME" << endl;
		if( addSchema )
			os << "where pk.TABLE_SCHEMA=?" << endl;
		os << "order by name, ordinal";
		return os.str();
	}
	α ProcSql( bool addSchema )ι->string{
		std::ostringstream os{ "select SPECIFIC_NAME from INFORMATION_SCHEMA.ROUTINES", std::ios::ate };
		if( addSchema )
			os << " where ROUTINE_SCHEMA=?";
		return os.str();
	}
}