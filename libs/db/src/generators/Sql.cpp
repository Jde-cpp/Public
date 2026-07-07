#include <jde/db/generators/Sql.h>
#include <jde/db/generators/Functions.h>
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
	α Sql::EmbedParams()Ι->string{ //display/logging only - not escaped for execution.
		string fullSql; fullSql.reserve( Text.size()+Params.size()*8 );
		char quote{};
		for( uint i=0, iParam=0; i<Text.size(); ++i ){
			const char ch = Text[i];
			if( !quote && (ch=='\'' || ch=='"') )
				quote = ch;
			else if( ch==quote )
				quote = 0;
			if( ch!='?' || quote || iParam==Params.size() ){
				fullSql.push_back( ch );
				continue;
			}
			auto& param = Params[iParam++];
			switch( param.Type() ){
				using enum EValue;
			case String: fullSql.append( '\''+Str::Replace(param.get_string(), "\'", "''")+'\'' ); break;
			case Null: fullSql.append( "null" ); break;
			case Bytes:
				if( param.get_bytes().size()==0 )
					fullSql.append( "Null" );
				else
					fullSql.append( '\''+Str::Encode64( param.get_bytes() )+'\'' );
				break;
			default:
				fullSql.append( param.ToString() );
			}
		}
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