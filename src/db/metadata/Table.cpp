#include <jde/db/metadata/Table.h>
#include <jde/db/metadata/Cluster.h>
#include <jde/db/metadata/Catalog.h>
#include <jde/db/metadata/Schema.h>
#include <jde/db/metadata/Column.h>
#include <jde/db/DataType.h>
#include <jde/db/DataSource.h>
#include <jde/Str.h>
#include "ddl/Syntax.h"

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

	α GetData( const jobject& j )ε->optional<jarray>{
		optional<jarray> data;
		if( auto kv = j.find("data"); kv!=j.end() ){
			data = jarray{};
			uint id = 0;
			for( let& row : kv->value().as_array() ){
				if( row.is_string() ){
					json jRow;
					jRow["id"] = id++;
					jRow["name"] = row.as_string();
					data->push_back( jRow );
				}
				else
					data->push_back( row );
			}
		}
		return data;
	}

	α GetFlagsData( const jobject& j )ε->flat_map<uint,string>{
		flat_map<uint,string> flagsData;
		if( auto kv = j.find("flagsData"); kv!=j.end() ){
			uint i=0;
			for( let& col : kv->value().as_array() )
				flagsData.emplace( 1 << i++, col.as_string() );
		}
		return flagsData;
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
		Data{ GetData(j) },
		HasCustomInsertProc{ j.contains("customInsertProc") ? j.at("customInsertProc").as_bool() : false },
		FlagsData{ GetFlagsData(j) },
		NaturalKeys{ GetNaturalKeys(j) },
		ParentChildMap{ GetParentChildMap(j,*this) },
		PurgeProcName{ j.contains("purgeProc") ? string{j.at("purgeProc").as_string()} : string{} },
		QLView{ j.contains("qlView") ? ms<View>(j.at("qlView").as_string()) : nullptr }
	{}

	α Table::Initialize( sp<DB::Schema> schema, sp<Table> self )ε->void{
		self->Schema = schema;
		if( QLView )
			QLView = schema->FindView( QLView->Name );
		for( let& c : Columns )
			c->Initiaize( self );
		//if mssql & schema is not default & ds schema!=config schema.
		if( !Syntax().CanSetDefaultSchema() && !Schema->Name.empty() && Schema->DS()->SchemaName()!=Schema->Name )
			DBName = Ƒ( "{}.", Schema->Name );
		if( Schema->Catalog->Cluster->ShouldPrefixTable )
			DBName += Schema->Name;
		DBName+=Name;
	}
	α Table::Syntax()Ι->const DB::Syntax&{ return Schema->Syntax(); }

	α Table::InsertProcName()Ι->string{
		let haveSequence = find_if( Columns, [](let& c){return c->IsIdentity;} )!=Columns.end();
		return !haveSequence || HasCustomInsertProc ? string{} : Ƒ( "{}_insert", DB::Schema::ToSingular(Name) );
	}

	α Table::GetExtendedFromTable()Ι->sp<Table>{//um_users return um_entities
		auto pPK = SurrogateKeys.size()>0 ? FindColumn( SurrogateKeys[0] ) : nullptr;
		return pPK && pPK->PKTable.size() ? Schema->TryFindTable(pPK->PKTable) : sp<Table>{};
	}

	α Table::FindColumnε( sv name )Ε->const Column&{
		auto pColumn = FindColumn( name );
		if( let pExtendedFrom = pColumn ? nullptr : GetExtendedFromTable(); pExtendedFrom )
			pColumn = pExtendedFrom->FindColumn( name );
		THROW_IF( !pColumn || !pColumn->Table, "Could not find column '{}'", name );
		return *pColumn;
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
		return pColumn ? Schema->TryFindTable( pColumn->PKTable ) : sp<Table>{};
		// let part = TableNamePart( *this, 0 );
		// return part.empty() ? sp<const Table>{} : schema.TryFindTableSuffix( Schema::ToPlural(part) );
	}

	α Table::HaveSequence()Ι->bool{
		return find_if( Columns, [](let& c){return c->IsIdentity;} )!=Columns.end();
	}

	α Table::ParentTable()Ι->sp<Table>{
		let pColumn = ParentColumn();
		return pColumn ? Schema->TryFindTable( pColumn->PKTable ) : sp<Table>{};
	}
	α Table::IsEnum()Ι->bool{
		return Columns.size()==2 && Columns[0]->Name=="id" && Columns[1]->Name=="name";
	}
}