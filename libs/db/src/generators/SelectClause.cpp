#include <jde/db/generators/SelectClause.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	α fromArray( const vector<sp<Column>>& cols, const string& alias )ι->vector<AliasCol>{
		vector<AliasCol> y; y.reserve( cols.size() );
		for( let& c : cols )
			y.emplace_back( alias, c );
		return y;
	}
	α tableCols( const View& t, const vector<string>& cols )ι->vector<AliasCol>{
		vector<AliasCol> y; y.reserve( cols.size() );
		for( auto& c : cols )
			y.emplace_back( t.GetColumnPtr(move(c)) );
		return y;
	}

	SelectClause::SelectClause( const vector<sp<Column>>& cols, const string& alias )ι:
		Columns{ fromArray(cols, alias) }
	{}

	SelectClause::SelectClause( const View& t, const vector<string>& cols )ι:
		Columns{ tableCols(t, cols) }
	{}

	α SelectClause::TryAdd( sp<DB::Column> c )ι->void{
		if( !FindColumn(c) )
			Columns.push_back( {c} );
	}
	α SelectClause::TryAdd( const AliasCol& c )ι->void{
		if( !FindColumn(c) )
			Columns.push_back( c );
	}

	α SelectClause::operator+=( SelectClause&& x )ι->SelectClause&{
		for( let& c : x.Columns )
			TryAdd( c );
		x.Columns.clear();
		return *this;
	}

	α SelectClause::FindColumn( const AliasCol& f )Ι->sp<Column>{
		auto p = find_if( Columns, [&](let& c){
			return c.Column->Name==f.Column->Name && c.Column->Table==f.Column->Table && f.Alias==c.Alias;
		});
		return p==Columns.end() ? nullptr : p->Column;
	}
	α SelectClause::FindColumn( const DB::Column& f )Ι->sp<Column>{
		auto p = find_if( Columns, [&](let& c){
			return c.Column->Name==f.Name && c.Column->Table==f.Table && c.Alias.empty();
		});
		return p==Columns.end() ? nullptr : p->Column;
	}
	α SelectClause::FindColumn( sv name )Ι->sp<Column>{
		auto p = find_if( Columns, [=](let& c){return c.Column->Name==name;} );
		return p==Columns.end() ? nullptr : p->Column;
	}

	α SelectClause::ToString( bool shouldAlias )Ι->string{
		string y{ "select" }; y.reserve( 128 );
		for( let& colAlias : Columns ){
			auto& c = colAlias.Column;
			auto alias = shouldAlias ? colAlias.Alias : "";
			if( shouldAlias && alias.empty() && c->Table )//count(*) won't have table
				alias = c->Table->DBName;

			string columnName = alias.size() ? alias + '.' + c->Name : c->Name;
			if( c->Type==EType::DateTime && c->Table )
				columnName = c->Table->Syntax().DateTimeSelect( move(columnName) )+' '+c->Name;
			y += ' '+ columnName + ',';
		}
		if( !y.empty() )
			y.pop_back();
		return y;
	}
}