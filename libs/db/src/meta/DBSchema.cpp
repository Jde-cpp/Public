#include <jde/db/meta/DBSchema.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/IDataSource.h>

#define let const auto
namespace Jde::DB{
	α GetAppSchemas( const jobject& schema, sp<Access::IAcl> authorizer )ε->flat_map<string,sp<AppSchema>>{
		flat_map<string,sp<AppSchema>> appSchemas;
		for( let& [name,appSchema] : schema )
			appSchemas.emplace( name, ms<AppSchema>(name, Json::AsObject(appSchema), authorizer) );
		return appSchemas;
	}

	DBSchema::DBSchema( sv name, const jobject& schema, sp<Access::IAcl> authorizer )ε:
		Name{ name },
		AppSchemas{ GetAppSchemas(schema, authorizer) }
	{}

	DBSchema::DBSchema( sv name, flat_map<string,sp<Table>> tables, sv prefix )ι:
		Name{ name },
		AppSchemas{ {"", ms<AppSchema>("", tables, prefix)} }
	{}

	α DBSchema::Initialize( sp<DB::Catalog> catalog, sp<DB::DBSchema> self )ε->void{
		self->Catalog = catalog;
		for_each(self->AppSchemas, [self](auto&& kv){ kv.second->Initialize(self,kv.second); });
	}

	α DBSchema::DS()Ε->sp<IDataSource>{
		if( !_dataSource ){
			auto ds = Catalog->DS();
			string name;
			let canSetSchema = ds->Syntax().CanSetDefaultSchema();
			try{
				name = ds->SchemaName();
			}
			catch( IException& ){
				if( !canSetSchema )
					throw;
				if( ds->SchemaNameConfig()==Name ){
					ds = ds->AtSchema( MySqlSyntax::Instance().SysSchema() );//no connection, can't figure out syntax.
					ds->ExecuteSync( {Ƒ("create schema {}", Name)} );
				}
			}
			_dataSource = name==Name || !canSetSchema
				? ds
				: ds->AtSchema( Name );
			Information{ ELogTags::Sql, "At Schema: {}", _dataSource->SchemaName() };
		}
		return _dataSource;
	}

	α DBSchema::FindAppSchema( str name )ι->sp<AppSchema>{
		auto p = AppSchemas.find( name );
		return p==AppSchemas.end() ? nullptr : p->second;
	}
}