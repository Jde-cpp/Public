#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/WhereClause.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>

#define let const auto

namespace Jde::DB{
	vector<Join> GetJoins( vec<sp<Table>>& tables, SL sl )ε{
		THROW_IF( tables.size()<2, "Invalid number of tables." );
		vector<Join> joins;
		for( uint i=1; i<tables.size(); ++i ){
			let& from = *tables[i-1]; THROW_IFSL( from.SurrogateKeys.size()!=1, "Invalid number of surrogate keys." );
			auto pk = from.SurrogateKeys[0];
			let& to = *tables[i];
			let fk = to.GetColumnPtr( pk->Name, sl );
			joins.emplace_back( pk, fk, true );
		}
		return joins;
	}
	FromClause::FromClause( vec<sp<Table>>& tables, SL sl )ε:
		SingleTable{ tables.size()==1 ? std::dynamic_pointer_cast<View>(tables[0]) : nullptr },
		Joins{ tables.size()!=1 ? GetJoins(tables, sl) : vector<Join>{} }
	{}
	FromClause::FromClause( const sp<Table>& t )ι:
		SingleTable{std::dynamic_pointer_cast<View>(t)}
	{}

	α FromClause::ToString()Ε->string{
		if( Joins.size()==0 )
			return "from "+SingleTable->DBName;

		let& syntax = Joins[0].From->Table->Schema->Syntax();
		string sql{ "from "+Joins[0].From->Table->DBName }; sql.reserve( 255 );
		for( let& join : Joins )
			sql += syntax.UsingClause( join );
		return sql;
	}

	α FromClause::operator+=( Join&& join )ι->FromClause&{
		SingleTable = nullptr;
		Joins.push_back(move(join)); return *this;
	}
	α FromClause::TryAdd( Join&& join )ι->void{
		if( !Contains(join.To->Table->Name) )
			*this += move(join);
	}

	α FromClause::Contains( sv tableName )ι->bool{
		bool y = SingleTable && SingleTable->Name==tableName;
		for( auto p = Joins.begin(); !y && p!=Joins.end(); ++p )
			y = p->From->Table->Name==tableName || p->To->Table->Name==tableName;
		return y;
	}

	α FromClause::GetColumnPtr( sv name, SL sl )Ε->sp<Column>{
		sp<Column> column = SingleTable ? SingleTable->FindColumn( name ) : nullptr;
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
		THROW_IFSL( !SingleTable && Joins.size()==0, "!SingleTable and Joins.size()==0" );
		return SingleTable ? SingleTable : Joins[0].From->Table;
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