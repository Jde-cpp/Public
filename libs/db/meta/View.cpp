#include <jde/db/meta/View.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/names.h>

#define let const auto

namespace Jde::DB{
	α ToColumns( const jobject& j )ε->vector<sp<Column>>{
		flat_map<uint8,sp<Column>> ordered;
		if( auto kv = j.find("columns"); kv!=j.end() ){
			uint8 defaultIndex = 128;
			for( let& nameCol : Json::AsObject(kv->value()) ){
				let name{ nameCol.key() };
				let& column{ Json::AsObject(nameCol.value()) };
				auto i = Json::FindNumber<uint8>( column, "i" ).value_or( defaultIndex++ );
				if( ordered.find(i)!=ordered.end() )
					i = defaultIndex++;
				ordered.emplace( Json::FindNumber<uint8>(column, "i").value_or(defaultIndex++), ms<Column>(Names::FromJson(name), column) );
			}
		}
		vector<sp<Column>> columns;
		for( let& [_,c] : ordered )
			columns.push_back( c );
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
		Columns{ ToColumns(j) },
		SurrogateKeys{ GetSurrogateKeys(Columns) }
	{}
	
	α View::Initialize( sp<DB::AppSchema> schema, sp<View> self )ε->void{
		self->Schema = schema;
		for( let& c : Columns )
			c->Initialize( self );

		//if mssql & schema is not default & ds schema!=config schema.
		bool representsDBTable = !DBName.empty();//db tables copy constructed will already have db name set.
		DBName.clear();
		if( !Syntax().CanSetDefaultSchema() && !Schema->Name.empty() && Schema->DS()->SchemaName()!=Schema->Name )
			DBName = Ƒ( "{}.", Schema->Name );
		if( !representsDBTable && Schema->Prefix.size() )
			DBName += Schema->Prefix;
		DBName+=Name;
	}

	α AsTable(const View& v)ε->const Table&{ 
		let p = dynamic_cast<const Table*>( &v ); THROW_IF( !p, "View '{}' is not a table.", v.Name );
		return *p;
	}
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
	
	α View::GetColumns( vector<string> names, SL sl )Ε->vector<sp<Column>>{
		vector<sp<Column>> columns;
		for( let& name : names )
			columns.push_back( GetColumnPtr(name, sl) );
		return columns;
	}

	α View::FindFK( sv pkTableName )Ι->sp<Column>{
		auto p = find_if( Columns, [&pkTableName](let& c){return c->PKTable && c->PKTable->Name==pkTableName;} );
		return p==Columns.end() ? sp<Column>{} : *p;
	}
	α View::FindPK()Ι->sp<Column>{
		return SurrogateKeys.size()==1 ? SurrogateKeys.front() : sp<Column>{};
	}

	α View::GetPK( SL sl )Ε->sp<Column>{
		auto p = FindPK(); THROW_IFSL( SurrogateKeys.size()!=1, "[{}]Expected 1 surrogate column, found: {}.", Name, SurrogateKeys.size() );
		return p;
	}
	
	α View::IsEnum()Ι->bool{
		return Columns.size()==2 && Columns[1]->Name=="name";
	}

	α View::Syntax()Ι->const DB::Syntax&{ return Schema->Syntax(); }

}