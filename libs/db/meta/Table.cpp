#include <jde/db/meta/Table.h>
#include <jde/db/names.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/DBSchema.h>
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
			if( let parentId = Json::FindSV(map, "parentId"); parentId ){
				auto parentColumn = view.GetColumnPtr( *parentId );
				auto childId = Json::AsSV( map, "childId" );
				auto childColumn = view.GetColumnPtr( childId );
				parentChildMap = make_tuple( parentColumn, childColumn );
			}
			else
				parentChildMap = make_tuple( nullptr, nullptr );//acl doesn't have parent/child.
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

	α Table::Initialize( sp<DB::AppSchema> schema, sp<Table> self )ε->void{
		self->Schema = schema;
		if( QLView )
			QLView = schema->GetViewPtr( QLView->Name );
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

	α Table::InsertProcName()Ι->string{
		let haveSequence = find_if( Columns, [](let& c){return c->IsSequence;} )!=Columns.end();
		return !haveSequence || HasCustomInsertProc ? string{} : Ƒ( "{}_insert", Names::ToSingular(Name) );
	}

	α Table::GetExtendedFromTable()Ι->sp<Table>{//groups return access_identities.  Assumes ExtendedFrom is 1st surrogate key.
		auto pFirstSK = SurrogateKeys.size()>0 ? SurrogateKeys.front() : nullptr;
		return pFirstSK ? pFirstSK->PKTable : sp<Table>{};
	}

	α Table::FindColumn( sv name )Ι->sp<Column>{
		auto pColumn = View::FindColumn( name );
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
	
	α Table::GetColumns( vector<string> names, SL sl )Ε->vector<sp<Column>>{
		vector<sp<Column>> columns;
		for( let& name : names )
			columns.push_back( GetColumnPtr(name, sl) );
		return columns;
	}

	α TableNamePart( const Table& table, uint8 index )ι->string{
		let name = table.NameWithoutType();//split returns temp
		let nameParts = Str::Split( name, '_' );
		return nameParts.size()>index ? Names::ToSingular( nameParts[index] ) : "";
	}
	α Table::Prefix()Ι->sv{ return Str::Split( Name, '_' )[0]; }
	α Table::NameWithoutType()Ι->sv{ let underscore = Name.find_first_of('_'); return underscore==string::npos ? Name : sv{Name.data()+underscore+1, Name.size()-underscore-1 }; }

	α Table::FKName()Ι->string{ return string{Names::ToSingular(NameWithoutType())}+"_id"; }
	α Table::JsonTypeName()Ι->string{
		auto name = Names::ToJson( Names::ToSingular(NameWithoutType()) );
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
		return QLView ? QLView->IsEnum() : View::IsEnum();
	}
}