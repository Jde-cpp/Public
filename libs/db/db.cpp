#include <jde/db/db.h>
#include <jde/framework/settings.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Schema.h>
#include "meta/ddl/CatalogDdl.h"
#include "meta/ddl/SchemaDdl.h"
#include "meta/IServerMeta.h"


#define let const auto

namespace Jde::DB{
	vector<sp<Cluster>> _clusters;
	α GetClusters()ε->vector<sp<Cluster>>{
		if( _clusters.empty() ){
			let& clusters = Settings::AsObject( "dbServers" );
			for( auto&& [name, value] : clusters ){
				_clusters.emplace_back( ms<Cluster>(name, Json::AsObject(value)) );
				Cluster::Initialize( _clusters.back() );
			}
			THROW_IF( _clusters.empty(), "No db servers found." );
		}
		return _clusters;
	}
}
namespace Jde{
	α DB::GetSchema( sv metaName )ε->sp<Schema>{
		for( auto& cluster : GetClusters() ){
			for( auto& catalog : cluster->Catalogs ){
				if( auto pSchema = catalog->FindSchema(metaName); pSchema )
					return pSchema;
			}
		}
		THROW( "Schema '{}' not found.", metaName );
	}


	α DB::SyncSchema( sv metaName )ε->sp<Schema>{
		auto schema = GetSchema( metaName );
		SchemaDdl::Sync( *schema );
		return schema;
	}
}

#ifndef PROD
namespace Jde::DB{
	α NonProd::Recreate( sp<Schema>& schema )ε->void{
		CatalogDdl::NonProd::Drop( *schema );
		SchemaDdl::Sync( *schema );
	}
#endif
}