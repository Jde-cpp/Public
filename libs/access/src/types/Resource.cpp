#include "Resource.h"
#include <jde/db/Value.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/WhereClause.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };

	Resource::Resource( DB::IRow& row )ι{
		PK = row.GetUInt16(0);
		Schema = row.MoveString(1);
		Target = row.MoveString(2);
		Filter = row.MoveString(3);
		Deleted = row.GetTimePointOpt(4);
	}
	Resource::Resource( ResourcePK pk, const jobject& j )ι:
		PK{ pk },
		Schema{ string{Json::FindDefaultSV(j, "schemaName")} },
		Target{ string{Json::FindDefaultSV(j, "target")} },
		Filter{ string{Json::FindDefaultSV(j, "criteria")} },
		Deleted{ Json::FindTimePoint(j, "deleted") }
	{}

	ResourceLoadAwait::ResourceLoadAwait( sp<DB::AppSchema> schema )ι:
		_schema{ schema }
	{}

	α ResourceLoadAwait::Load()ι->DB::RowAwait::Task{
		ResourcePermissions y;
		try{
			auto resourceTable = _schema->GetTablePtr( "resources" );
			let ds = _schema->DS();
			const DB::WhereClause where{ resourceTable->GetColumnPtr("schema_name"), DB::Value{_schema->Name} };
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
		let sql = Ƒ( "insert into {0}(name,target,allowed,description,schema_name,created,deleted) values(?,?,?,?,?,{1},{1})", resourceTable.DBName, schema.Syntax().UtcNow() );
		vector<Value> params{ {}, {}, {}, Value{"From installation"}, Value{schema.Name} };
		for( let& [name, value] : ops ){
			params[0] = Value{ name };
			params[1] = Value{ DB::Names::ToJson(name) };
			params[2] = value;
			schema.DS()->Execute( sql, params );
		}
	}
}