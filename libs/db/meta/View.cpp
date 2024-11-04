#include <jde/db/meta/View.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/names.h>

#define let const auto

namespace Jde::DB{
	α toColumns( const jobject& j )ε->vector<sp<Column>>{
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

	α getMap( const jobject& j, const View& view )ε->optional<View::ParentChildMap>{
		auto jmap = Json::FindObject( j, "map" );
		return jmap
			? View::ParentChildMap{view.GetColumnPtr(Json::AsSV(*jmap, "parentId")), view.GetColumnPtr(Json::AsSV(*jmap, "childId")) }
			: optional<View::ParentChildMap>{};
	}

	α getSurrogateKeys( const vector<sp<Column>>& columns )ι->vector<sp<Column>>{
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

constexpr Access::ERights DefaultOps{ Access::ERights::Create | Access::ERights::Read | Access::ERights::Update | Access::ERights::Delete | Access::ERights::Purge | Access::ERights::Administer };
	View::View( sv name, const jobject& o )ε:
		Name{name},
		Columns{ toColumns(o) },
		HasCustomInsertProc{ Json::FindBool(o, "customInsertProc").value_or(false) },
		IsFlags{ o.contains("flagsData") },
		Map{ getMap(o,*this) },
		QLView{ o.contains("qlView") ? ms<View>( Json::AsString(o,"qlView")) : nullptr },
		SurrogateKeys{ getSurrogateKeys(Columns) },
		Operations{ Json::FindArray(o, "ops") ? Access::ToRights(Json::AsArray(o, "ops")) : DefaultOps }
	{}

	α View::Initialize( sp<DB::AppSchema> schema, sp<View> self )ε->void{
		self->Schema = schema;
		for( let& c : Columns )
			c->Initialize( self );

		if( QLView )
			QLView = schema->GetViewPtr( QLView->Name );

		if( Map )
			Map->Parent->PKTable->Children.emplace_back( self );

		//if mssql & schema is not default & ds schema!=config schema.
		bool representsDBTable = !DBName.empty();//db tables copy constructed will already have db name set.
		DBName.clear();
		if( !Syntax().CanSetDefaultSchema() && !Schema->Name.empty() && Schema->DS()->SchemaName()!=Schema->Name )
			DBName = Ƒ( "{}.", Schema->Name );
		if( !representsDBTable && Schema->Prefix.size() )
			DBName += Schema->Prefix;
		DBName+=Name;
	}

	α View::ChildTable()Ι->sp<View>{
		return Map && Map->Child ? Map->Child->PKTable : sp<Table>{};
	}
	α View::ParentTable()Ι->sp<View>{
		return Map && Map->Parent ? Map->Parent->PKTable : sp<Table>{};
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

	α View::GetPK( SL sl )Ε->sp<Column>{
		auto p = FindPK(); THROW_IFSL( SurrogateKeys.size()!=1, "[{}]Expected 1 surrogate column, found: {}.", Name, SurrogateKeys.size() );
		return p;
	}
	α View::GetSK0(SL sl)Ε->sp<Column>{
		THROW_IFSL( SurrogateKeys.size()==0, "No surrogate keys for '{}'.", Name );
		return SurrogateKeys[0];
	}

	α View::FindFK( sv pkTableName )Ι->sp<Column>{
		auto p = find_if( Columns, [&pkTableName](let& c){return c->PKTable && c->PKTable->Name==pkTableName;} );
		return p==Columns.end() ? sp<Column>{} : *p;
	}
	α View::FindPK()Ι->sp<Column>{
		return SurrogateKeys.size()==1 ? SurrogateKeys.front() : sp<Column>{};
	}

	α View::HaveSequence()Ι->bool{
		return find_if( Columns, [](let& c){return c->IsSequence;} )!=Columns.end();
	}

	α View::InsertProcName()Ι->string{
		let haveSequence = find_if( Columns, [](let& c){return c->IsSequence;} )!=Columns.end();
		return !haveSequence || HasCustomInsertProc ? string{} : Ƒ( "{}_insert", Names::ToSingular(Name) );
	}

	α View::IsEnum()Ι->bool{
		return QLView ? QLView->IsEnum() : Columns.size()==2 && Columns[1]->Name=="name";
	}
	α View::JsonTypeName()Ι->string{
		auto name = Names::ToJson( Names::ToSingular(Name) );
		if( name.size() )
			name[0] = (char)std::toupper( name[0] );
		return name;
	}

	α View::Syntax()Ι->const DB::Syntax&{ return Schema->Syntax(); }
}
namespace Jde{
	α DB::AsTable(sp<View> v)ι->sp<Table>{ return dynamic_pointer_cast<Table>(v); }
	α DB::AsTable(const View& v)ε->const Table&{
		let p = dynamic_cast<const Table*>( &v ); THROW_IF( !p, "View '{}' is not a table.", v.Name );
		return *p;
	}
}