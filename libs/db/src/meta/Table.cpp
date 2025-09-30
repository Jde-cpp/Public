#include <jde/db/meta/Table.h>
#include <jde/fwk/str.h>
#include <jde/db/names.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/DBSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Syntax.h>

#define let const auto

namespace Jde::DB{
	using std::endl;
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
		NaturalKeys{ GetNaturalKeys(j) },
		PurgeProcName{ j.contains("purgeProc") ? string{j.at("purgeProc").as_string()} : string{} },
		Extends{ j.contains("extends") ? ms<DB::Table>(Json::AsString(j,"extends")) : nullptr }
	{}

	α Table::Initialize( sp<DB::AppSchema> schema, sp<Table> self )ε->void{
		if( Extends )
			Extends = schema->GetTablePtr( Extends->Name );
		View::Initialize( schema, self );
	}

	α Table::FindColumn( sv name )Ι->sp<Column>{
		auto pColumn = View::FindColumn( name );
		if( !pColumn && Extends )
			pColumn = Extends->FindColumn( name );
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

	α Table::FKName()Ι->string{ return string{Names::ToSingular(Name)}+"_id"; }
}