#include "ResourceLoadAwait.h"
//#include <jde/db/Value.h>
#include <jde/db/IDataSource.h>
//#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include "../accessInternal.h"
//#include <jde/db/generators/FromClause.h>
//#include <jde/db/generators/WhereClause.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	α ResourceLoadAwait::Load()ι->QL::QLAwait::Task{
		ResourcePermissions y;
		try{
			let ql = Ƒ( "resources( schemaName:[{}] ){{ id schemaName target criteria deleted }}", Str::Join(_schemaNames, ",", true) );
			let resources = co_await *_qlServer->Query( ql, _executer );
			for( let& value : Json::AsArray(resources) ){
				auto resource = Resource{ Json::AsObject(value) };
				y.Resources.emplace( resource.PK, move(resource) );
			}

			let qlPermissions = Ƒ( "permissionRights{{ id allowed denied resource( schemaName:[{}] ){{id}} }}", Str::Join(_schemaNames, ",", true) );
			let permissions = co_await *_qlServer->Query( qlPermissions, _executer );
			for( let& value : Json::AsArray(permissions) ){
				let permission = Permission{ Json::AsObject(value) };
				y.Permissions.emplace( permission.PK, move(permission) );
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

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
		let sql = Ƒ( "insert into {0}(name,target,allowed,description,schema_name,created,deleted) values(?,?,?,?,?,{1},{1})", resourceTable.DBName, schema.Syntax().UtcNow() );
		vector<Value> params{ {}, {}, {}, Value{"From installation"}, Value{schema.Name} };
		for( let& [name, value] : ops ){
			params[0] = Value{ name };
			params[1] = Value{ DB::Names::ToJson(name) };
			params[2] = value;
			schema.DS()->Execute( sql, params );
		}
	}
	/*	α ResourceLoadAwait::Load()ι->DB::RowAwait::Task{
		ResourcePermissions y;
		try{
			auto resourceTable = _schema->GetTablePtr( "resources" );
			let ds = _schema->DS();
			const DB::WhereClause where{ resourceTable->GetColumnPtr("schema_name"), _schema->Name };
			auto statement = DB::SelectSql( {"resource_id","schema_name","target","criteria", "deleted"}, resourceTable, where );
			auto rows = co_await ds->SelectCo( move(statement) );
			for( auto& row : rows )
				y.Resources.emplace( row->GetUInt16(0), Resource{*row} );

			auto permissionRights = _schema->GetTablePtr( "permission_rights" );
			vector<string> columns{"permission_id","resource_id", "allowed", "denied", "deleted"};
			DB::FromClause from{ {permissionRights->GetColumnPtr("resource_id"), resourceTable->GetPK(), true} };
			statement = DB::SelectSql( columns, from, where );
			rows = co_await ds->SelectCo( move(statement) );
			for( let& row : rows ){
				let pk = row->GetUInt16(0);
				let resourceId = row->GetUInt16(1);
				y.Permissions.emplace( pk, Permission{pk, resourceId, (ERights)row->GetUInt16(2), (ERights)row->GetUInt16(3)} );
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
*/
}