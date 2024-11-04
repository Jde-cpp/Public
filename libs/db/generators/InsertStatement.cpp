#include <jde/db/generators/InsertStatement.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto
namespace Jde::DB{
	α InsertStatement::Move()ι->DB::Sql{
		if( Values.empty() )
			return {};

		let& table = *Values.begin()->first->Table;
		let procName = table.InsertProcName();
		return procName.size() ? Proc( procName ) : Insert( table.DBName );
	}

	α InsertStatement::Proc( str procName )ι->DB::Sql{
		DB::Sql sql; sql.Text.reserve( 128 );
		sql.Text = procName+"(?";
		for( auto& [column,value] : Values ){
			if( sql.Params.size() )
				sql.Text += ",?";
			sql.Params.emplace_back( move(value) );
		}
		sql.Text+=")";
		return sql;
	}

	α InsertStatement::Insert( str tableName )ι->DB::Sql{
		IsStoredProc = false;
		DB::Sql sql; sql.Text.reserve( 256 );
		sql.Text = "insert into "+tableName+" (";
		string params{"?"};
		for( auto& [column,value] : Values ){
			if( sql.Params.size() ){
				sql.Text += ",";
				params += ",?";
			}
			sql.Text += column->Name;
			sql.Params.emplace_back( move(value) );
		}
		sql.Text+=") values("+params+")";
		return sql;
	}
	α InsertStatement::SetValue( sp<Column> column, Value value )ε->void{
		auto p = find_if( Values, [column](auto& p){ return p.first==column; } );
		THROW_IF( p==Values.end(), "Could not find column '{}'", column->Name );
		p->second = move(value);
	}
}