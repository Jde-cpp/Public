#include <jde/db/generators/UpdateClause.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	α UpdateClause::Move()ι->DB::Sql{
		DB::Sql sql; sql.Text.reserve( 128 );
		if( !Values.size() || Where.Empty() )
			return sql;

		ASSERT( Values.begin()->first->Table );
		let& table = *Values.begin()->first->Table;
		sql.Text += "update ";
		bool first{ true };//not Params.empty(): a null or "$now" column pushes no param, so the column after one would re-emit the "set " prefix.
		for( auto&& [column,value] : Values ){ //C16: flat_map yields a prvalue proxy of references, so auto&& - mutable - the params below are moved out, not copied.
			if( first )
				sql.Text += table.SqlName()+" set ";
			else
				sql.Text += ", ";
			first = false;
			string valueText{ '?' };
			if( value.is_null() )
				valueText = "null";
			else if( value.is_string() && value.get_string()=="$now" )
				valueText = column->Table->Syntax().UtcNow();
			else
				sql.Params.push_back( move(value) );
			sql.Text += column->Name + " = "+valueText;
		}
		//ql-review3 #45: the statement table's *own* column.  Table::FindColumn resolves through Extends, so an `update access_users`
		//picked up identities' `updated` and every update of a users-owned column failed with "no such column: updated" - after the
		//parent half had already committed, untransacted.
		if( auto updated = table.View::FindColumn("updated"); updated && Values.find(updated)==Values.end() )
			sql.Text += ", "+updated->Name + " = " + string{ table.Schema->Syntax().UtcNow() };
		sql.Params.insert( sql.Params.end(), std::make_move_iterator(Where.Params().begin()), std::make_move_iterator(Where.Params().end()) ); //C16: Move() may take the where params with it.
		sql.Text += '\n' + Where.Move();
		return sql;
	}
	α UpdateClause::Add( sp<Column> column, Value value )ε->void{
		Values.try_emplace( column, move(value) );
	}
}