#include <jde/db/metadata/Schema.h>
#include <jde/db/metadata/Catalog.h>
#include <jde/db/metadata/Column.h>
#include <jde/db/metadata/Table.h>
#include <jde/db/DataSource.h>
#include "ddl/Syntax.h"

#define var const auto
namespace Jde::DB{
	α ParseTables( const jobject& schema )ε->flat_map<string,sp<Table>>{
		flat_map<string,sp<Table>> tables;
		for( var& [name,table] : schema )
			tables.emplace( name, ms<Table>(name, table.as_object()) );
		return tables;
	}

	Schema::Schema( sv name, const jobject& schema )ε:
		Name{ name },
		Tables{ ParseTables(schema) }
	{}

	α Schema::Initialize( sp<DB::Catalog> catalog, sp<DB::Schema> self )ι->void{
		self->Catalog = catalog;
		for_each(self->Tables, [self](auto&& kv){ kv.second->Initialize(self,kv.second); });
	}
	α Schema::DS()ε->sp<IDataSource>{
		if( !_dataSource ){
			_dataSource = DBName.empty() || Catalog->DS()->SchemaName()==DBName || !Syntax().CanSetDefaultSchema()
				? Catalog->DS()
				: Catalog->DS()->AtSchema( DBName );
		}
		return _dataSource;
	}

	α Schema::Syntax()Ι->const DB::Syntax&{ return Catalog->Syntax(); }

	α Schema::FindDefTable( const Table& t1, const Table& t2 )Ι->sp<Table>{
		sp<Table> result;
		var singularT1 = t1.FKName();
		var singularT2 = t2.FKName();
		for( var& [name,pTable] : Tables ){
			var pParentTable = pTable->ParentTable();
			var pChildTable = pTable->ChildTable();
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