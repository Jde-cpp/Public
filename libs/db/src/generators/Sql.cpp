#include <jde/db/generators/Sql.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/generators/WhereClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>

#define let const auto

namespace Jde::DB{
	α Sql::operator+=( WhereClause&& where )ι->Sql&{
		Text += where.Move();
		Params.insert( Params.end(), std::make_move_iterator(where.Params().begin()), std::make_move_iterator(where.Params().end()) );
		return *this;
	}
	α Sql::EmbedParams()Ι->string{
		string fullSql; fullSql.reserve( Text.size()+Params.size()*2 );
		uint iStart=0;
		for( uint i=Text.find('?'), iParam=0; iParam<Params.size() && i<Text.size(); iStart=i+1, i=Text.find('?', i+1) ){
			fullSql.append( Text, iStart, i-iStart );
			auto& param = Params[iParam++];
			if( param.Type()==EValue::String )
				fullSql.append( '\''+Str::Replace(param.get_string(), "\'", "''")+'\'' );
			else if( param.Type()==EValue::Null )
				fullSql.append( "null" );
			else
				fullSql.append( param.ToString() );
		}
		if( iStart<Text.size() )
			fullSql.append( Text, iStart, Text.size()-iStart );
		return fullSql;
	}
}
namespace Jde{
	α DB::SelectSql( const vector<sp<Column>>& columns, FromClause from, WhereClause where, SL sl )ε->Sql{
		string sql; sql.reserve( 1023 );
		sql += "select ";
		bool haveDeleted{};
		for( let& c : columns ){
			let columnName = c->Table->DBName+'.'+c->Name;
			if( c->Type==EType::DateTime ){
				sql += c->Table->Syntax().DateTimeSelect( columnName )+' '+c->Name + ',';
				if( c->Name=="deleted" )
					haveDeleted = true;
			}
			else
				sql += columnName + ',';
		}
		if( !haveDeleted )
			from.SetActive( where, sl );
		sql[sql.size()-1] = '\n';
		sql += from.ToString()+'\n';
		sql += where.Move();
		return {move(sql), move(where.Params())};
	}

	α DB::SelectSql( vec<string> columns, FromClause from, WhereClause where, SL sl )ε->Sql{
		vector<sp<Column>> cols;
		for( let& column : columns )
			cols.push_back( from.GetColumnPtr(column, sl) );
		return SelectSql( cols, move(from), where );
	}

	α DB::SelectSKsSql( sp<Table> table )ε->Sql{
		return SelectSql( table->SurrogateKeys, {table}, {} );
	}
}