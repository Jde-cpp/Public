#include "Resource.h"
#include <jde/db/Value.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/await/RowAwait.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/WhereClause.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };

	ResourceLoadAwait::ResourceLoadAwait( sp<DB::AppSchema> schema )ι:
		Schema{ schema }
	{}

	Ω loadResources( ResourceLoadAwait& await )ι->DB::RowAwait::Task{
		flat_map<ResourcePK,Resource> resources;
		let& schema = *await.Schema;
		try{
			sp<DB::Table> resourceTable = schema.GetTablePtr( "resources" );
			let ds = resourceTable->Schema->DS();
			const DB::WhereClause where{ resourceTable->GetColumnPtr("schema_name"), DB::Value{schema.Name} };
			auto statement = DB::SelectSql( {"resource_id","schema","target","filter"}, resourceTable, where );
			auto rows = co_await ds->SelectCo( move(statement) );
			for( let& row : rows ){
				let pk = row->GetUInt16(0);
				resources.emplace( pk, Resource{pk, row->GetString(1), row->GetString(2), row->GetString(3)} );
			}
			sp<DB::Table> resourceRightsTable = schema.GetTablePtr( "resource_rights" );
			vector<string> columns{"permission_id","resource_id", "allowed", "denied"};
			DB::FromClause from{ {resourceTable, resourceRightsTable} };
			statement = DB::SelectSql( columns, from, where );
			rows = co_await ds->SelectCo( move(statement) );
			for( let& row : rows ){
				let pk = row->GetUInt16(0);
				let resourceId = row->GetUInt16(1);
				if( auto p = resources.find(resourceId); p!=resources.end() )
					p->second.Permissions.emplace( pk, Permission{pk, (ERights)row->GetUInt16(2), (ERights)row->GetUInt16(3)} );
				else
					Warning{ _tags, "[{}]Resource not found for permission: {}.", resourceId, pk };
			}
			await.Resume( move(resources) );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α ResourceLoadAwait::Suspend()ι->void{
		loadResources( *this );
	}

	α GetSchema()ε->sp<DB::AppSchema>;
	α Resources::Sync()ε->void{
		using DB::Value;
		let& schema = *GetSchema();
		let& resourceTable = schema.GetTable( "resources" );
		if( schema.DS()->Scaler<uint>( Ƒ("select count(*) from {} where schema_name=?", resourceTable.DBName), {Value{schema.Name}} )>0 )
			return;
		flat_map<string,Value> ops;
		for( let& [name,table] : schema.Tables ){
			if( !empty(table->Operations) )
				ops.emplace( name, Value{underlying(table->Operations)} );
		}
		let sql = Ƒ( "insert into {0}(name,target,rights,description,schema_name,created,deleted) values(?,?,?,?,?,{1},{1})", resourceTable.DBName, schema.Syntax().UtcNow() );
		vector<Value> params{ {}, {}, {}, Value{"From installation"}, Value{schema.Name} };
		for( let& [name, value] : ops ){
			params[0] = Value{ name };
			params[1] = Value{ DB::Names::ToJson(name) };
			params[2] = value;
			schema.DS()->Execute( sql, params );
		}
	}
}