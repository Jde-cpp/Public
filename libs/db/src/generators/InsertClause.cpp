#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto
namespace Jde::DB{
	Ω getParams( vector<Value>&& values )ι->vector<std::pair<sp<Column>,Value>>{
		vector<std::pair<sp<Column>,Value>> params;
		for( auto& v : values )
			params.emplace_back( std::make_pair(sp<Column>{}, move(v)) );
		return params;
	}
	InsertClause::InsertClause( sv name, vector<Value>&& params )ι:
		Values{ getParams(move(params)) },
		_procName{ name }
	{}

	α InsertClause::Move()ι->DB::Sql{
		DB::Sql sql;
		if( _procName )
			sql = Proc( *_procName );
		else if( Values.size() && Values.begin()->first ){
			let& table = *Values.begin()->first->Table;
			let procName = _procName.value_or( table.InsertProcName() );
			sql = IsStoredProc && procName.size() ? Proc( procName ) : Insert( table.DBName );
		}
		sql.IsProc = IsStoredProc;
		return sql;
	}
	α InsertClause::Add( sp<Column> column, Value::Underlying value )ι->void{
		Values.emplace_back( make_pair(column, move(value)) );
	}

	α InsertClause::Add( Value::Underlying value )ι->void{
		Values.emplace_back( make_pair(sp<Column>{}, move(value)) );
	}

	α InsertClause::SequenceColumn()Ι->sp<Column>{
		auto table = Values.size() ? Values.begin()->first->Table : nullptr;
		return table ? table->SequenceColumn() : nullptr;
	}

	α InsertClause::Proc( str procName )ι->DB::Sql{
		DB::Sql sql; sql.Text.reserve( 128 );
		sql.Text = procName+"(?";
		for( auto& [_,value] : Values ){
			if( sql.Params.size() )
				sql.Text += ",?";
			sql.Params.emplace_back( move(value) );
		}
		sql.Text+=")";
		return sql;
	}

	α InsertClause::Insert( str tableName )ι->DB::Sql{
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
	α InsertClause::SetValue( sp<Column> column, Value value )ε->void{
		auto p = find_if( Values, [column](auto& p){ return p.first==column; } );
		THROW_IF( p==Values.end(), "Could not find column '{}'", column->Name );
		p->second = move(value);
	}
}