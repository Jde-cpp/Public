#include <jde/db/generators/SelectClause.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	α SelectClause::TryAdd( sp<DB::Column> c )ι->void{
		if( !FindColumn(*c) )
			Columns.push_back( c );
	}
	α SelectClause::TryAdd( sp<DB::Column> c, string tableAlias )ι->sp<DB::Column>{
		if( tableAlias.size() ){
			c = ms<Column>( c->Name );
			c->Table = ms<Table>( tableAlias );
		}
		TryAdd( c );
		return c;
	}

	α SelectClause::operator+=( SelectClause&& x )ι->SelectClause&{
		for( let& c : x.Columns )
			TryAdd( c );
		x.Columns.clear();
		return *this;
	}
	α SelectClause::FindColumn( const DB::Column& f )Ι->sp<Column>{
		auto p = find_if( Columns, [=](let& c){return c->Name==f.Name && (!c->Table || !f.Table || c->Table->Name==f.Table->Name);} );
		return p==Columns.end() ? nullptr : *p;
	}
	α SelectClause::FindColumn( sv name )Ι->sp<Column>{
		auto p = find_if( Columns, [=](let& c){return c->Name==name;} );
		return p==Columns.end() ? nullptr : *p;
	}

	α SelectClause::ToString()Ι->string{
		string s{ "select" }; s.reserve( 128 );
		for( let& c : Columns ){
			string columnName = c->Table //count(*) won't have table
				? c->Table->DBName + '.' + c->Name
				: c->Name;
			if( c->Type==EType::DateTime && c->Table )
				columnName = c->Table->Syntax().DateTimeSelect( columnName )+' '+c->Name;
			s += ' '+ columnName + ',';
		}
		if( !s.empty() )
			s.pop_back();
		return s;
	}
}