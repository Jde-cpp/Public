#include <jde/db/metadata/View.h>
#include <jde/db/metadata/Column.h>
#include <jde/db/metadata/Table.h>

#define let const auto

namespace Jde::DB{
	α GetColumns( const jobject& j, const vector<string>& surrogateKeys )ε->vector<sp<Column>>{
		vector<sp<Column>> columns;
		if( auto kv = j.find("columns"); kv!=j.end() ){
			for( const jvalue& column : kv->value().as_array() ){
				let c = column.as_object();
				const string columnName{ c.at("name").as_string() };
				bool sk = find(surrogateKeys, columnName)!=surrogateKeys.end();
				// if( auto sk =  ){
				// 	p->IsId = true;
				// 	p->Updateable = false;
				// }
				columns.push_back( ms<Column>(c, sk) );
			}
		}
		return columns;
	}

	α GetSurrogateKeys( const jobject& j )->vector<string>{
		vector<string> keys;
		if( auto kv = j.find("surrogateKeys"); kv!=j.end() ){
			for( let& sk : kv->value().as_array() )
				keys.emplace_back( sk.as_string() );
		}
		return keys;
	}

	View::View( sv name, const jobject& j )noexcept(false):
		Name{name},
		SurrogateKeys{ GetSurrogateKeys(j) },
		Columns{ GetColumns(j, SurrogateKeys) }
	{}

	α View::FindColumn( sv name )Ι->sp<Column>{
		auto pColumn = find_if( Columns, [&name](let& c){return c->Name==name;} );
		return pColumn==Columns.end() ? sp<Column>{} : *pColumn;
	}
	α View::FindColumnε( sv name, SL sl )Ε->sp<Column>{
		auto pColumn = FindColumn( name ); THROW_IFSL( !pColumn, "Could not find column '{}'", name );
		return pColumn;
	}

	α View::SurrogateColumns()Ε->vector<sp<Column>>{
		vector<sp<Column>> surrogateColumns;
		for( let& sk : SurrogateKeys )
			surrogateColumns.push_back( FindColumnε(sk) );
		return surrogateColumns;
	}
}