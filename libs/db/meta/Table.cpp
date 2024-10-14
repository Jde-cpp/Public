#include <jde/db/meta/Table.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Schema.h>
#include <jde/db/meta/Column.h>
//#include <jde/db/Value.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>
#include <jde/framework/str.h>


#define let const auto

namespace Jde::DB{
	using std::endl;
	α GetParentChildMap( const jobject& j, const View& view )ε->optional<tuple<sp<Column>,sp<Column>>>{
		optional<tuple<sp<Column>,sp<Column>>> parentChildMap;
		if( auto kv = j.find("map"); kv!=j.end() ){
			let& map = kv->value();
			let parentId = map.at("parentId").as_string();
			auto pParentColumn = view.FindColumn( parentId ); THROW_IF( !pParentColumn, "Could not find parentId: '{}' in '{}'", string{parentId}, view.Name );
			auto childId = DB::Schema::FromJson( map.at("childId").as_string() );
			auto pChildColumn = view.FindColumn( childId ); THROW_IF( !pChildColumn, "Could not find childId: '{}' in '{}'", string{childId}, view.Name );
			parentChildMap = make_tuple( pParentColumn, pChildColumn );
		}
		return parentChildMap;
	}

	α GetNaturalKeys( const jobject& j )ε->vector<vector<string>>{
		vector<vector<string>> naturalKeys;
		if( auto kv = j.find("naturalKeys"); kv!=j.end() ){
			for( let& nk : kv->value().as_array() ){
				vector<string> columns;
				for( let& col : nk.as_array() )
					columns.emplace_back( col.as_string() );
				naturalKeys.push_back( move(columns) );
			}
		}
		return naturalKeys;
	}

	Table::Table( sv name, const jobject& j )ε:
		View{ name, j },
		HasCustomInsertProc{ j.contains("customInsertProc") ? j.at("customInsertProc").as_bool() : false },
		IsFlags{ j.contains("flagsData") },
		NaturalKeys{ GetNaturalKeys(j) },
		ParentChildMap{ GetParentChildMap(j,*this) },
		PurgeProcName{ j.contains("purgeProc") ? string{j.at("purgeProc").as_string()} : string{} },
		QLView{ j.contains("qlView") ? ms<View>(j.at("qlView").as_string()) : nullptr }
	{}

	α Table::Initialize( sp<DB::Schema> schema, sp<Table> self )ε->void{
		self->Schema = schema;
		if( QLView )
			QLView = schema->GetViewPtr( QLView->Name );
		for( let& c : Columns )
			c->Initialize( self );

		//if mssql & schema is not default & ds schema!=config schema.
		if( !Syntax().CanSetDefaultSchema() && !Schema->Name.empty() && Schema->DS()->SchemaName()!=Schema->Name )
			DBName = Ƒ( "{}.", Schema->Name );
		if( Schema->Catalog->Cluster->ShouldPrefixTable )
			DBName += Schema->Name;
		DBName+=Name;
	}
	α Table::Syntax()Ι->const DB::Syntax&{ return Schema->Syntax(); }

	α Table::InsertProcName()Ι->string{
		let haveSequence = find_if( Columns, [](let& c){return c->IsSequence;} )!=Columns.end();
		return !haveSequence || HasCustomInsertProc ? string{} : Ƒ( "{}_insert", DB::Schema::ToSingular(Name) );
	}

	α Table::GetExtendedFromTable()Ι->sp<Table>{//groups return access_identities.  Assumes ExtendedFrom is 1st surrogate key.
		auto pFirstSK = SurrogateKeys.size()>0 ? SurrogateKeys.front() : nullptr;
		return pFirstSK ? pFirstSK->PKTable : sp<Table>{};
	}

	α Table::FindColumn( sv name )Ι->sp<Column>{
		auto pColumn = FindColumn( name );
		if( let pExtendedFrom = pColumn ? nullptr : GetExtendedFromTable(); pExtendedFrom )
			pColumn = pExtendedFrom->FindColumn( name );
		return pColumn;
	}

	α Table::GetColumn( sv name, SL sl )Ε->const Column&{
		return *GetColumnPtr( name, sl );
	}

	α Table::GetColumnPtr( sv name, SL sl )Ε->sp<Column>{
		auto pColumn = FindColumn( name ); THROW_IFSL( !pColumn, "[{}.{}]Could not find column.", Name, name );
		return pColumn;
	}

/*	α ColumnStartingWith( const Table& table, sv part )ι->string{
		let pColumn = find_if( table.Columns, [&part](let& c){return c->Name.starts_with(part);} );
		return pColumn==table.Columns.end() ? string{} : pColumn->Name;
	}*/

	α TableNamePart( const Table& table, uint8 index )ι->string{
		let name = table.NameWithoutType();//split returns temp
		let nameParts = Str::Split( name, '_' );
		return nameParts.size()>index ? DB::Schema::ToSingular( nameParts[index] ) : "";
	}
	α Table::Prefix()Ι->sv{ return Str::Split( Name, '_' )[0]; }
	α Table::NameWithoutType()Ι->sv{ let underscore = Name.find_first_of('_'); return underscore==string::npos ? Name : sv{Name.data()+underscore+1, Name.size()-underscore-1 }; }

	α Table::FKName()Ι->string{ return string{Schema::ToSingular(NameWithoutType())}+"_id"; }
	α Table::JsonTypeName()Ι->string{
		auto name = Schema::ToJson( Schema::ToSingular(NameWithoutType()) );
		if( name.size() )
			name[0] = (char)std::toupper( name[0] );
		return name;
	}

	α Table::ChildTable()Ι->sp<Table>{
		let pColumn = ChildColumn();
		return pColumn ? pColumn->PKTable : sp<Table>{};
		// let part = TableNamePart( *this, 0 );
		// return part.empty() ? sp<const Table>{} : schema.TryFindTableSuffix( Schema::ToPlural(part) );
	}

	α Table::HaveSequence()Ι->bool{
		return find_if( Columns, [](let& c){return c->IsSequence;} )!=Columns.end();
	}

	α Table::ParentTable()Ι->sp<Table>{
		let pColumn = ParentColumn();
		return pColumn ? pColumn->PKTable : sp<Table>{};
	}
	α Table::IsEnum()Ι->bool{
		return Columns.size()==2 && Columns[1]->Name=="name";
	}
}