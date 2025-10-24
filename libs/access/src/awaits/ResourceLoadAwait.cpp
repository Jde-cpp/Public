#include "ResourceLoadAwait.h"
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include "../accessInternal.h"
#include <jde/ql/IQL.h>
#include <jde/fwk/chrono.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	α ResourceLoadAwait::Load()ι->QL::QLAwait<jarray>::Task{
		ResourcePermissions y;
		try{
			string ql{ "resources" }; ql.reserve( 1024 );
			string schemaInput;
			if( _schemas.size() ){
				schemaInput.reserve( 256 );
				schemaInput+="(schemaName:[";
				for( let& schema : _schemas )
					schemaInput += Ƒ( "\"{}\",", schema->Name );
				schemaInput.back() = ']';
				schemaInput += ')';
			}
			let resources = co_await *_qlServer->QueryArray( Ƒ("resources{}{{ id schemaName target criteria deleted }}", schemaInput), {}, _executer );
			for( auto&& value : resources ){
				auto resource = Resource{ Json::AsObject(move(value)) };
				y.Resources.emplace( resource.PK, move(resource) );
			}

			let qlPermissions = Ƒ( "permissionRights{{ id allowed denied resource{}{{id}} }}", move(schemaInput) );
			let permissions = co_await *_qlServer->QueryArray( qlPermissions, {}, _executer );
			for( auto&& value : permissions ){
				let permission = Permission{ Json::AsObject(move(value)) };
				ASSERT(permission.ResourcePK);
				y.Permissions.emplace( permission.PK, move(permission) );
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α ResourceSyncAwait::Sync()ι->TAwait<jvalue>::Task{
		try{
			for( let& schema : _schemas ){
				auto q = Ƒ( "resources( schemaName:[\"{}\"] ){{target deleted}}", schema->Name );
				auto existing = Json::AsArray( co_await *_qlServer->Query(move(q), {}, _executer) );
				flat_set<string> targets;
				for( auto& resource : existing )
					targets.emplace( Json::AsString(Json::AsObject(resource), "target") );

				for( let& [_,table] : schema->Tables ){
					auto jsonName = DB::Names::ToJson( table->Name );
					if( empty(table->Operations) || targets.contains(jsonName) )
						continue;

					auto create = Ƒ( "createResource( schemaName:\"{}\", name:\"{}\", target:\"{}\", allowed:{}, description:\"From installation\" ){{id}}",
						schema->Name, table->Name, move(jsonName), underlying(table->Operations) );
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