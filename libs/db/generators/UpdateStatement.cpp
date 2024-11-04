#include <jde/db/generators/UpdateStatement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	α UpdateStatement::Move()ι->DB::Sql{
		DB::Sql sql; sql.Text.reserve( 128 );
		if( !Values.size() || Where.Empty() )
			return sql;
		
		ASSERT( Values.begin()->first->Table );
		let& table = *Values.begin()->first->Table;
		sql.Text += "update ";
		for( let& [column,value] : Values ){
			if( sql.Params.empty() )
				sql.Text += table.DBName+" set ";
			else
				sql.Text += ", ";
			string valueText{ '?' };
			if( value.is_null() )
				valueText = "null";
			else if( value.is_string() && value.get_string()=="$now" )
				valueText = ToSV( column->Table->Syntax().UtcNow() );
			else
				sql.Params.push_back( value );
			sql.Text += column->Name + " = "+valueText;
		}
		if( auto updated = table.FindColumn("updated"); updated && Values.find(updated)==Values.end() )
			sql.Text += ", "+updated->Name + " = " + ToStr( table.Schema->Syntax().UtcNow() );
		sql.Params.insert( sql.Params.end(), Where.Params().begin(), Where.Params().end() );
		sql.Text += '\n' + Where.Move();
		return sql;
	}
}