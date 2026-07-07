#include "ResourceLoadAwait.h"
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include "../accessInternal.h"
#include <jde/ql/IQL.h>
#include <jde/fwk/chrono.h>

#define let const auto
namespace Jde::Access{
	Ω getSchemaName( const sp<DB::AppSchema>& schema, const string& opcServerInstance )ι->string{
		return opcServerInstance.empty() ? schema->Name : Ƒ( "{}.{}", schema->Name, opcServerInstance );
	}

	α ResourceLoadAwait::Load()ι->QL::QLAwait<jarray>::Task{
		ResourcePermissions y;
		try{
			jarray schemaNames;
			bool loadAll{};
			for( let& schema : _schemas ){
				if( schema->Name=="app" )
					loadAll = true;
				schemaNames.push_back( {getSchemaName(schema, _opcServerInstance)} );
			}
			auto vars = loadAll ? jobject{} : jobject{ {"schemaNames", move(schemaNames)} };
			auto input = loadAll ? "" : "(schemaName:$schemaNames)";
			let resources = co_await *_qlServer->QueryArray( Ƒ("resources{}{{ id schemaName target criteria deleted }}", input), vars, _executer );
			for( auto&& value : resources ){
				auto resource = Resource{ Json::AsObject(move(value)) };
				y.Resources.emplace( resource.PK, move(resource) );
			}

			let permissions = co_await *_qlServer->QueryArray( Ƒ("permissionRights{{ id allowed denied resource{}{{id}} }}", input), move(vars), _executer );
			for( auto&& value : permissions ){
				let permission = Permission{ Json::AsObject(move(value)) };
				ASSERT(permission.ResourcePK);
				y.Permissions.emplace( permission.PK, move(permission) );
			}
			Resume( move(y) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ResourceSyncAwait::Sync()ι->TAwait<jvalue>::Task{
		try{
			for( let& schema : _schemas ){
				let schemaName = getSchemaName(schema, _opcServerInstance);
				auto q = Ƒ( "resources( schemaName:[\"{}\"] ){{target deleted}}", schemaName );
				auto existing = Json::AsArray( co_await *_qlServer->Query(move(q), {}, _executer) );
				flat_set<string> targets;
				for( auto& resource : existing )
					targets.emplace( Json::AsString(Json::AsObject(resource), "target") );

				for( let& [_,table] : schema->Tables ){
					auto jsonName = DB::Names::ToJson( table->Name );
					if( empty(table->Operations) || targets.contains(jsonName) )
						continue;

					auto create = Ƒ( "createResource( schemaName:\"{}\", name:\"{}\", target:\"{}\", allowed:{}, description:\"From installation\" ){{id}}",
						schemaName, table->Name, move(jsonName), underlying(table->Operations) );
					let resourceId = QL::AsId<UserPK::Type>( co_await *_qlServer->Query(move(create), {}, _executer) );
					co_await *_qlServer->Query( Ƒ("deleteResource( id:{} )", resourceId), {}, _executer );
				}
			}
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}