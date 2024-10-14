#include <jde/db/meta/View.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Schema.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::DB{
	α GetColumns( const jobject& j )ε->vector<sp<Column>>{
		flat_map<uint8,sp<Column>> ordered;
		if( auto kv = j.find("columns"); kv!=j.end() ){
			uint8 defaultIndex = 128;
			for( let& nameCol : kv->value().as_object() ){
				let name = nameCol.key();
				let& column = nameCol.value().as_object();
				ordered.emplace( column.contains("i") ? column.at("i").as_uint64() : (uint8)defaultIndex++, ms<Column>(Schema::FromJson(name), column) );
			}
		}
		vector<sp<Column>> columns;
		return columns;
	}
	α GetSurrogateKeys( const vector<sp<Column>>& columns )ι->vector<sp<Column>>{
		flat_map<uint8,sp<Column>> skColumns;
		for( let& c : columns ){
			if( c->SKIndex.has_value() )
				skColumns.emplace( *c->SKIndex, c );
		}
		vector<sp<Column>> y;
		for( let& [_,c] : skColumns )
			y.push_back( c );
		return y;
	}

	View::View( sv name, const jobject& j )noexcept(false):
		Name{name},
		Columns{ GetColumns(j) },
		SurrogateKeys{ GetSurrogateKeys(Columns) }
	{}

	α View::FindColumn( sv name )Ι->sp<Column>{
		auto pColumn = find_if( Columns, [&name](let& c){return c->Name==name;} );
		return pColumn==Columns.end() ? sp<Column>{} : *pColumn;
	}
	α View::GetColumn( sv name, SL sl )Ε->const Column&{
		return *GetColumnPtr( name, sl );
	}
	α View::GetColumnPtr( sv name, SL sl )Ε->sp<Column>{
		auto pColumn = FindColumn( name ); THROW_IFSL( !pColumn, "[{}.{}]Could not find column.", Name, name );
		return pColumn;
	}

	α View::FindPK()Ι->sp<Column>{
		return SurrogateKeys.size()==1 ? SurrogateKeys.front() : sp<Column>{};
	}

	α View::GetPK( SL sl )Ε->sp<Column>{
		auto p = FindPK(); THROW_IFSL( SurrogateKeys.size()!=1, "[{}]Expected 1 surrogate column, found: {}.", Name, SurrogateKeys.size() );
		return p;
	}
}