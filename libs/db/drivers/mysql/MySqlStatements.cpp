#include "MySqlStatements.h"

namespace Jde::DB::MySql{
	using std::endl;
	α Ddl::ColumnSql( sv tablePrefix, bool addTable )ι->string{
		std::ostringstream os;
		os << "select	t.TABLE_NAME, COLUMN_NAME, ORDINAL_POSITION, COLUMN_DEFAULT, IS_NULLABLE, COLUMN_TYPE, CHARACTER_MAXIMUM_LENGTH, EXTRA='auto_increment' is_identity, 0 is_id, NUMERIC_PRECISION, NUMERIC_SCALE" << endl
			<< "from		INFORMATION_SCHEMA.TABLES t" << endl
			<< "\tjoin INFORMATION_SCHEMA.COLUMNS c using(TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME)" << endl
			<< "\twhere t.TABLE_SCHEMA=?" << endl
			<< "\tand\tt.TABLE_TYPE in ('BASE TABLE', 'VIEW')" << endl;
		if( addTable )
			os << "\tand t.TABLE_NAME=:table_name" << endl;
		else if( tablePrefix.size() )
			os << "\tand t.TABLE_NAME like ?" << endl;
		os << "order by t.TABLE_NAME, ORDINAL_POSITION" << endl;
		return os.str();
	}

	α Ddl::ForeignKeySql( bool addSchema )ι->string{
		std::ostringstream os;
		os << "select	fk.CONSTRAINT_NAME name, fk.TABLE_NAME foreign_table, fk.COLUMN_NAME fk, pk.TABLE_NAME primary_table, pk.COLUMN_NAME pk, pk.ORDINAL_POSITION ordinal" << endl
			<< "from INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS con" << endl
			<< "  join INFORMATION_SCHEMA.KEY_COLUMN_USAGE fk on con.CONSTRAINT_NAME=fk.CONSTRAINT_NAME" << endl
			<< "  join INFORMATION_SCHEMA.KEY_COLUMN_USAGE pk on pk.CONSTRAINT_NAME COLLATE utf8_general_ci=con.UNIQUE_CONSTRAINT_NAME and pk.ORDINAL_POSITION=fk.ORDINAL_POSITION and pk.TABLE_NAME=con.REFERENCED_TABLE_NAME" << endl;
		if( addSchema ){
			os << "where pk.TABLE_SCHEMA=?" << endl;
			os << "  and fk.TABLE_SCHEMA=?" << endl;
		}
		os << "order by name, ordinal";
		return os.str();
	}

	α Ddl::IndexSql( sv tablePrefix, bool addTable )ι->string{
		std::ostringstream os;
		os << "SELECT TABLE_NAME, INDEX_NAME, COLUMN_NAME, NON_UNIQUE, SEQ_IN_INDEX FROM INFORMATION_SCHEMA.STATISTICS WHERE TABLE_SCHEMA = ?";
		if( addTable )
			os << " and TABLE_NAME=?";
		else if( tablePrefix.size() )
			os << " and TABLE_NAME like ?";
		os << " order by TABLE_NAME, INDEX_NAME, SEQ_IN_INDEX";
		return os.str();
	}

	α Ddl::ProcSql( bool addSchema )ι->string{
		std::ostringstream os{ "select SPECIFIC_NAME from INFORMATION_SCHEMA.ROUTINES", std::ios::ate };
		if( addSchema )
			os << " where ROUTINE_SCHEMA=?";
		return os.str();
	}
}