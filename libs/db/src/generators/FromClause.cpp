#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/WhereClause.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>

#define let const auto

namespace Jde::DB{
	α getJoins( vec<sp<Table>>& tables, SL sl )ε->vector<Join>{
		THROW_IF( tables.empty(), "tables.empty()" );
		vector<Join> joins;
		for( uint i=1; i<tables.size(); ++i ){
			let& from = *tables[i-1]; THROW_IFSL( from.SurrogateKeys.size()!=1, "Invalid number of surrogate keys." );
			auto pk = from.SurrogateKeys[0];
			let& to = *tables[i];
			let fk = to.GetColumnPtr( pk->Name, sl );
			joins.emplace_back( pk, fk, true );
		}
		if( joins.empty() )
			joins.push_back( {tables[0]->Columns[0]} );

		return joins;
	}
	FromClause::FromClause( const sp<View>& v )ι:
		Joins{ {v->Columns[0]} }
	{};
	FromClause::FromClause( vec<sp<Table>>& tables, SL sl )ε:
		Joins{ getJoins(tables, sl) }
	{}
	FromClause::FromClause( const sp<Table>& t )ε:
		Joins{ {t->GetPK()} }
	{}

	α FromClause::ToString()Ε->string{
		if( Joins.size()==0 )
			throw Exception( "Joins.size()==0" );

		let& syntax = Joins[0].From->Table->Schema->Syntax();
		string sql{ "from "+Joins[0].From->Table->DBName }; sql.reserve( 255 );
		if( Joins[0].FromAlias.size() )
			sql += " "+Joins[0].FromAlias;
		if( Joins[0].To ){
			for( let& join : Joins )
				sql += syntax.UsingClause( join );
		}
		return sql;
	}

	α FromClause::operator+=( Join&& join )ι->FromClause&{
		if( Joins.size()==0 )
			Joins.push_back( move(join) );
		else if( !Joins[0].To ){
			Joins[0].To = join.To;
			Joins[0].Inner = join.Inner;
			Joins[0].ToAlias = join.ToAlias;
		}
		else
			Joins.push_back( move(join) );
		return *this;
	}
	// Change a single column to a join.
	α FromClause::Add( sp<Column> from, sp<Column> to, bool inner )ι->void{
		Joins.push_back( {from, to, inner} );
	}

	α FromClause::TryAdd( Join&& join )ι->void{
		if( !Contains(join.To->Table->Name) )
			*this += move(join);
	}

	α FromClause::Contains( sv tableName )ι->bool{
		bool contains{};
		for( auto p = Joins.begin(); !contains && p!=Joins.end(); ++p )
			contains = p->From->Table->Name==tableName || p->To->Table->Name==tableName;
		return contains;
	}

	α FromClause::GetColumnPtr( sv name, SL sl )Ε->sp<Column>{
		sp<Column> column{};
		for( uint i=0; !column && i<Joins.size(); ++i ){
			let& join = Joins[i];
			if( i==0 )
				column = join.From->Table->FindColumn( name );
			if( !column )
				column = join.To->Table->FindColumn( name );
		}
		THROW_IFSL( !column, "Column '{}' not found.", name );
		return column;
	}

	α FromClause::GetFirstTable( SL sl )Ε->sp<View>{
		THROW_IFSL( Joins.size()==0, "!SingleTable and Joins.size()==0" );
		return Joins[0].From->Table;
	}
	//add 'deleted is null'
	α FromClause::SetActive( WhereClause& where, SL sl )ε->void{
		auto table = GetFirstTable( sl );
		if( let pDeleted=table->FindColumn("deleted"); pDeleted ){
			where.Add( pDeleted, nullptr );
			if( pDeleted->Table->Name!=table->Name && table->SurrogateKeys.size()>1 )
				TryAdd( {table->SurrogateKeys[0], pDeleted->Table->GetPK(), true} ); //table=members, pDeleted=identities.  Extension tables have extension_id first.
		}
	}
}