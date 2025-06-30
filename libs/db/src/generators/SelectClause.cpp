#include <jde/db/generators/SelectClause.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Object.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	α fromArray( const vector<sp<Column>>& cols, const string& alias )ι->vector<Object>{
		vector<Object> y; y.reserve( cols.size() );
		for( let& c : cols )
			y.emplace_back( AliasCol{alias, c} );
		return y;
	}
	α tableCols( const View& t, str alias, const vector<string>& cols )ι->vector<Object>{
		vector<Object> y; y.reserve( cols.size() );
		for( auto& c : cols )
			y.emplace_back( AliasCol{alias, t.GetColumnPtr(move(c))} );
		return y;
	}

	SelectClause::SelectClause( sp<Column> c )ι:
		Columns{ AliasCol{{},c} }
	{}

	SelectClause::SelectClause( const vector<sp<Column>>& cols, const string& alias )ι:
		Columns{ fromArray(cols, alias) }
	{}

	SelectClause::SelectClause( const View& t, str alias, const vector<string>& cols )ι:
		Columns{ tableCols(t, move(alias), cols) }
	{}

	SelectClause::SelectClause( AliasCol aliasCol )ι:
		Columns{aliasCol}
	{}

	α SelectClause::TryAdd( Object c )ι->void{
		if( !FindColumn(c) )
			Columns.push_back( {c} );
	}
	α SelectClause::TryAdd( const AliasCol& c )ι->void{
		if( !FindColumn(c) )
			Columns.push_back( c );
	}
	α SelectClause::TryAdd( const sp<Column>& c )ι->void{
		if( !FindColumn(*c) )
			Columns.push_back( AliasCol{c} );
	}

	α SelectClause::operator+=( SelectClause&& x )ι->SelectClause&{
		for( let& c : x.Columns )
			TryAdd( c );
		x.Columns.clear();
		return *this;
	}

	α SelectClause::FindColumn( const Object& a )Ι->const Object*{
		auto p = find_if( Columns, [&](let& b){
			return a==b;
		});
		return p==Columns.end() ? nullptr : &*p;
	}
	α SelectClause::FindColumn( const DB::Column& a )Ι->sp<Column>{
		auto p = find_if( Columns, [&](let& c){
			return c.index()==underlying(EObject::AliasColumn) && *get<AliasCol>(c).Column==a;
		});
		return p==Columns.end() ? nullptr : get<AliasCol>(*p).Column;
	}
	α SelectClause::FindColumn( sv name )Ι->sp<Column>{
		auto p = find_if( Columns, [=](let& o){
			auto c = get_if<AliasCol>( &o );
			return c && c->Column->Name==name;
		});
		return p==Columns.end() ? nullptr : get<AliasCol>(*p).Column;
	}

	α SelectClause::ToString( bool shouldAlias )Ι->string{
		string y{ "select" }; y.reserve( 128 );
		for( let& o : Columns ){
			if( o.index()==underlying(EObject::AliasColumn) ){
				let& colAlias = get<AliasCol>(o);
				auto& c = colAlias.Column;
				auto alias = shouldAlias ? colAlias.Alias : "";
				if( shouldAlias && alias.empty() && c->Table )//count(*) won't have table
					alias = c->Table->DBName;

				string columnName = alias.size() ? alias + '.' + c->Name : c->Name;
				if( c->Type==EType::DateTime && c->Table )
					columnName = c->Table->Syntax().DateTimeSelect( move(columnName) )+' '+c->Name;
				y += ' '+ columnName + ',';
			}
			else
				y += ' '+ DB::ToString(o) + ',';
		}
		if( !y.empty() )
			y.pop_back();
		return y;
	}
}