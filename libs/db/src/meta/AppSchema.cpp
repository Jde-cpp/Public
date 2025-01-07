#include <jde/db/meta/AppSchema.h>
#include <jde/framework/io/json.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/meta/DBSchema.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>
#define let const auto

namespace Jde::DB{
	α GetTables( const jobject& jtables )ε->flat_map<string,sp<Table>>{
		flat_map<string,sp<Table>> tables;
		for( let& [jname,table] : jtables ){
			let name = Names::FromJson(jname);
			tables.emplace( name, ms<Table>(name, Json::AsObject(table)) );
		}
		return tables;
	}

	α GetViews( const jobject& jviews )ε->flat_map<string,sp<View>>{
		flat_map<string,sp<View>> views;
		for( let& [jname,view] : jviews ){
			let name = Names::FromJson(jname);
			views.emplace( name, ms<Table>(name, Json::AsObject(view)) );
		}
		return views;
	}

	AppSchema::AppSchema( sv name, sv prefix, const jobject& meta, sp<Access::IAcl> authorizer )ε:
		Name{ name },
		Authorizer{ authorizer },
		Prefix{ prefix },
		Tables{ GetTables(Json::AsObject(meta,"tables")) },
		Views{ meta.contains("views") ? GetViews(Json::AsObject(meta.at("views"))) : flat_map<string,sp<View>>{} }
	{}

	AppSchema::AppSchema( sv name, const jobject& appSchema, sp<Access::IAcl> authorizer )ε:
		AppSchema{ name, Json::FindDefaultSV(appSchema,"prefix"), Json::ReadJsonNet(Json::AsString(appSchema,"meta")), authorizer }
	{}

	α AppSchema::Initialize( sp<DB::DBSchema> db, sp<AppSchema> self )ε->void{
		self->DBSchema = db;
		for_each(self->Tables, [self](auto&& kv){ kv.second->Initialize(self,kv.second); });
		for_each(self->Views, [self](auto&& kv){ kv.second->Initialize(self,kv.second); });
	}
	α AppSchema::ConfigPath()Ι->string{
		let catalog = DBSchema->Catalog;
		let cluster = catalog->Cluster;
		return Ƒ( "/dbServers/{}/catalogs/{}/schemas/{}/{}", cluster->ConfigName, catalog->Name, DBSchema->Name, Name );
	}
	α AppSchema::DS()Ε->sp<IDataSource>{
		return DBSchema->DS();
	}
	α AppSchema::ResetDS()Ι->void{ DBSchema->ResetDS(); }
	α AppSchema::Syntax()Ι->const DB::Syntax&{
		let isPhysical = DBSchema->IsPhysical();
		let& syntax = isPhysical ? DS()->Syntax() : DB::Syntax::Instance();
		return syntax;
	}

	α AppSchema::FindTable( str name )Ι->sp<Table>{
		let y = Tables.find( name );
		return y==Tables.end() ? nullptr : y->second;
	}
	α AppSchema::GetTable( str name, SL sl )Ε->const Table&{
		return *GetTablePtr( name, sl );
	}
	α AppSchema::GetTablePtr( str name, SL sl )Ε->sp<Table>{
		let y = FindTable( name ); THROW_IFSL( !y, "[{}.{}]Could not find table.", Name, name );
		return y;
	}

	α AppSchema::FindView( str name )Ι->sp<View>{
		auto kv = Views.find( name );
		return kv==Views.end() ? FindTable(name) : kv->second;
	}
	α AppSchema::GetView( str name, SL sl )ε->const View&{
		return *GetViewPtr( name, sl );
	}
	α AppSchema::GetViewPtr( str name, SL sl )ε->sp<View>{
		let y = FindView( name ); THROW_IFSL( !y, "Could not find view '{}'", name );
		return y;
	}

	α AppSchema::FindDefTable( const Table& t1, const Table& t2 )Ι->sp<Table>{
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