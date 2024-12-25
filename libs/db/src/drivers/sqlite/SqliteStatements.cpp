#include "SqliteStatements.h"

namespace Jde::DB::Sqlite
{
	α Sql::ColumnSql( sv tablePrefix, bool addTable )ι->string{
		std::ostringstream os;
		os	<< "select tables.name table_name, columns.name column_name, columns.column_id, constraints.definition, columns.is_nullable, types.name, columns.max_length, is_identity, case when indexes.object_id is null then 0 else 1 end, columns.precision, columns.scale" << endl
			<< "from sys.schemas" << endl
			<< "join sys.tables on tables.schema_id=schemas.schema_id" << endl
			<< "join sys.columns on tables.object_id=columns.object_id" << endl
			<< "join sys.types on columns.system_type_id=types.system_type_id" << endl
			<< "left join sys.indexes on tables.object_id=indexes.object_id and is_primary_key=1" << endl
			<< "left join sys.default_constraints constraints on columns.default_object_id=constraints.object_id" << endl
			<< "\twhere schemas.name=?" << endl;
		if( addTable )
			os << "\tand t.TABLE_NAME=:table_name" << endl;
		else if( tablePrefix.size() )
			os << "\tand tables.name like ?" << endl;
		os << "order by tables.name, columns.column_id" << endl;
		return os.str();
	}
}