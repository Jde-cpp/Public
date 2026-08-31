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
			for( let& schema : _schemas )
				schemaNames.push_back( {getSchemaName(schema, _opcServerInstance)} );
			auto vars = _allSchemas ? jobject{} : jobject{ {"schemaNames", move(schemaNames)} };//explicit now - "app" in the list used to mean this.
			auto input = _allSchemas ? "" : "(schemaName:$schemaNames)";
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
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	α ResourceSyncAwait::Sync()ι->TAwait<jvalue>::Task{
		try{
			for( let& schema : _schemas ){
				let schemaName = getSchemaName(schema, _opcServerInstance);
				auto q = Ƒ( "resources( schemaName:[\"{}\"] ){{id target deleted description}}", schemaName );
				auto existing = Json::AsArray( co_await *_qlServer->Query(move(q), {}, _executer) );
				flat_set<string> targets;
				for( auto& value : existing ){
					let& resource = Json::AsObject( value );
					targets.emplace( Json::AsString(resource, "target") );
					//A row a sync created and never got to disable:  the disable is a second call, and a failure between the two left the
					//table denying every non-System user - for good, since a target with a row was then skipped here (access-review3 #24).
					//Its signature is the sync's own description and not one right on it; an operator who enabled a resource granted something.
					let deleted = resource.if_contains( "deleted" );
					if( !(deleted && deleted->is_null()) || Json::FindDefaultSV(resource, "description")!="From installation" )
						continue;
					let id = Json::AsNumber<ResourcePK>( resource, "id" );
					if( Json::AsArray(co_await *_qlServer->Query(Ƒ("permissionRights( resourceId:{} ){{ id }}", id), {}, _executer)).empty() ){
						INFOT( ELogTags::Access, "[{}.{}]resource {} is active with no rights on it - disabling it, as the installation that created it meant to.", schemaName, Json::AsString(resource, "target"), id );
						co_await *_qlServer->Query( Ƒ("deleteResource( id:{} )", id), {}, _executer );
					}
				}

				//Tables with ops, plus the resources the schema declares without a table behind them (the OpcServer's
				//`nodeIds`) - one list so both kinds get the identical row.
				vector<std::pair<string,Access::ERights>> declared;
				declared.reserve( schema->Tables.size()+schema->Resources.size() );
				for( let& [_,table] : schema->Tables )
					declared.emplace_back( table->Name, table->Operations );
				for( let& [name,ops] : schema->Resources )
					declared.emplace_back( name, ops );

				for( let& [name,ops] : declared ){
					auto jsonName = DB::Names::ToJson( name );
					if( empty(ops) || targets.contains(jsonName) )
						continue;

					auto create = Ƒ( "createResource( schemaName:\"{}\", name:\"{}\", target:\"{}\", allowed:{}, description:\"From installation\" ){{id}}",
						schemaName, name, move(jsonName), underlying(ops) );
					let resourceId = QL::AsId<UserPK::Type>( co_await *_qlServer->Query(move(create), {}, _executer) );
					co_await *_qlServer->Query( Ƒ("deleteResource( id:{} )", resourceId), {}, _executer );
				}
			}
			Resume();
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}