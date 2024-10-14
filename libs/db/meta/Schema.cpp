#include <jde/db/meta/Schema.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>

#define let const auto
namespace Jde::DB{
	α GetTables( const jobject& jtables )ε->flat_map<string,sp<Table>>{
		flat_map<string,sp<Table>> tables;
		for( let& [name,table] : jtables )
			tables.emplace( name, ms<Table>(name, Json::AsObject(table)) );
		return tables;
	}

	α GetViews( const jobject& jviews )ε->flat_map<string,sp<View>>{
		flat_map<string,sp<View>> views;
		for( let& [name,view] : jviews )
			views.emplace( name, ms<Table>(name, Json::AsObject(view)) );
		return views;
	}

	Schema::Schema( sv name, const jobject& schema )ε:
		Name{ name },
		Tables{ GetTables(Json::AsObject(schema,"tables")) },
		Views{ GetViews(Json::AsObject(schema,"views")) }
	{}

	Schema::Schema( sv name, flat_map<string,sp<Table>> tables )ι:
		Name{ name },
		Tables{ tables }
	{}

	α Schema::Initialize( sp<DB::Catalog> catalog, sp<DB::Schema> self )ι->void{
		self->Catalog = catalog;
		for_each(self->Tables, [self](auto&& kv){ kv.second->Initialize(self,kv.second); });
	}

	α Schema::DS()Ε->sp<IDataSource>{
		if( !_dataSource ){
			_dataSource = DBName.empty() || Catalog->DS()->SchemaName()==DBName || !Syntax().CanSetDefaultSchema()
				? Catalog->DS()
				: Catalog->DS()->AtSchema( DBName );
		}
		return _dataSource;
	}

	α Schema::Syntax()Ι->const DB::Syntax&{ return Catalog->Syntax(); }

	α Schema::FindTable( str name )Ι->sp<Table>{
		let y = Tables.find( name );
		return y==Tables.end() ? nullptr : y->second;
	}

	α Schema::GetTable( str name, SL sl )Ε->const Table&{
		return *GetTablePtr( name, sl );
	}

	α Schema::GetTablePtr( str name, SL sl )Ε->sp<Table>{
		let y = FindTable( name ); THROW_IF( !y, "[{}.{}]Could not find table.", Name, name );
		return y;
	}

	α Schema::FindView( str name )Ι->sp<View>{
		auto kv = Views.find( name );
		return kv==Views.end() ? FindTable(name) : kv->second;
	}

	α Schema::GetView( str name, SL sl )ε->const View&{
		return *GetViewPtr( name, sl );
	}

	α Schema::GetViewPtr( str name, SL sl )ε->sp<View>{
		let y = FindView( name ); THROW_IF( !y, "Could not find view '{}'", name );
		return y;
	}

	α Schema::FindDefTable( const Table& t1, const Table& t2 )Ι->sp<Table>{
		sp<Table> result;
		let singularT1 = t1.FKName();
		let singularT2 = t2.FKName();
		for( let& [name,pTable] : Tables ){
			let pParentTable = pTable->ParentTable();
			let pChildTable = pTable->ChildTable();
			if( pParentTable && pChildTable
				&& (  (t1.Name==pParentTable->Name && t2.Name==pChildTable->Name)
						||(t2.Name==pParentTable->Name && t1.Name==pChildTable->Name) ) ){
				result = pTable;
				break;
			}
		}
		return result;
	}
 }