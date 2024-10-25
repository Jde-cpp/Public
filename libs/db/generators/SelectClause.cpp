#include <jde/db/generators/SelectClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	α SelectClause::Add( sp<DB::Column> c )ι->void{
		if( !FindColumn(c->Name) )
			Columns.push_back( c );
	}
	α SelectClause::FindColumn( sv name )Ι->sp<Column>{
		auto p = find_if( Columns, [=](auto& c){return c->Name==name;} );
		return p==Columns.end() ? nullptr : *p;
	}
	α SelectClause::ToString()Ι->string{
		string s{ "select" }; s.reserve( 128 );
		for( let& c : Columns ){
			if( c->Table )//count(*) won't have table
				s += ' '+c->Table->DBName + '.' + c->Name + ',';
			else
				s += ' '+c->Name + ',';
		}
		if( !s.empty() )
			s.pop_back();
		return s;
	}
}