#include "ResourceLoadAwait.h"
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include "../accessInternal.h"
#include <jde/ql/IQL.h>
#include <jde/framework/chrono.h>

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
			let resources = co_await *_qlServer->QueryArray( Ƒ("resources{}{{ id schemaName target criteria deleted }}", schemaInput), _executer );
			for( auto&& value : resources ){
				auto resource = Resource{ Json::AsObject(move(value)) };
				y.Resources.emplace( resource.PK, move(resource) );
			}

			let qlPermissions = Ƒ( "permissionRights{{ id allowed denied resource{}{{id}} }}", move(schemaInput) );
			let permissions = co_await *_qlServer->QueryArray( qlPermissions, _executer );
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

	α loadExisting( const string& schemaName, sp<QL::IQL> qlServer, UserPK executor )ε->flat_set<string>{
		auto q = Ƒ("resources( schemaName:[\"{}\"] ){{target deleted}}", schemaName);
		auto existing = BlockAwait<TAwait<jarray>, jarray>( *qlServer->QueryArray( move(q), executor ) );
		flat_set<string> targets;
		for( auto& resource : existing )
			targets.emplace( Json::AsString(Json::AsObject(resource), "target") );
		return targets;
	}
	α createExisting( string&& query, sp<QL::IQL> qlServer, UserPK executor )ε->jvalue{
		return BlockAwait<TAwait<jvalue>, jvalue>( *qlServer->Query(move(query), executor) );
	}

	α Resources::Sync( const vector<sp<DB::AppSchema>> schemas, sp<QL::IQL> qlServer, UserPK executor )ε->void{
		using DB::Value;
		for( let& schema : schemas ){
			auto existing = loadExisting( schema->Name, qlServer, executor );

			for( let& [_,table] : schema->Tables ){
				auto jsonName = DB::Names::ToJson( table->Name );
				if( empty(table->Operations) || existing.contains(jsonName) )
					continue;

				auto q = Ƒ( "createResource( schemaName:\"{}\", name:\"{}\", target:\"{}\", allowed:{}, description:\"From installation\" ){{id}}",
					schema->Name, table->Name, move(jsonName), underlying(table->Operations) );
				let result = createExisting( move(q), qlServer, executor );
				BlockAwait<TAwait<jvalue>, jvalue>( *(qlServer->Query(Ƒ("deleteResource( id:{} )", QL::AsId<UserPK::Type>(result)), executor)) );
			}
		}
	}
}