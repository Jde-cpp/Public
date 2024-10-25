#include <jde/db/generators/UpdateStatement.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	α UpdateStatement::Move()ι->DB::Sql{
		DB::Sql sql; sql.Text.reserve( 128 );
		if( !Values.size() || Where.Empty() )
			return sql;

		sql.Text += "update ";
		for( let& [column,value] : Values ){
			if( sql.Params.empty() )
				sql.Text += column->Table->DBName+" set ";
			else
				sql.Text += ", ";

			sql.Text += column->Name + " = ?";
			sql.Params.push_back( value );
		}
		sql += move( Where );
		return sql;
	}
}