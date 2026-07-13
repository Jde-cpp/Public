#include <jde/db/db.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/process/dll.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/AppSchema.h>
#include "jde/fwk/log/logTags.h"
#include "jde/fwk/usings.h"
#include "meta/ddl/CatalogDdl.h"
#include "meta/ddl/SchemaDdl.h"
#include "meta/IServerMeta.h"
#include "c_api.h"

#define let const auto

namespace Jde::DB{
	vector<sp<Cluster>> _clusters;
	fs::path _scriptPath;
	Ω getClusters( sp<Access::IAcl> authorize, optional<jobject> dbSettings=nullopt )ε->vector<sp<Cluster>>{
		if( _clusters.empty() ){
			let& clusters = dbSettings.value_or( Settings::AsObject( "/dbServers") );
			for( auto&& [name, value] : clusters ){
				if( value.is_object() ){ //vs sync script dir
					_clusters.emplace_back( ms<Cluster>(name, value.get_object(), authorize) );
					Cluster::Initialize( _clusters.back() );
				}
			}
			THROW_IF( _clusters.empty(), "No db servers found." );
		}
		return _clusters;
	}
}
namespace Jde{
	class DataSourceApi{
		DllHelper _dll;
	public:
		DataSourceApi( fs::path&& path ):
			_dll{ move(path) },
			GetDataSourceFunction{ _dll["GetDataSource"] }
		{}
		decltype(GetDataSource) *GetDataSourceFunction;

		α Emplace( const jobject& config )ε->sp<DB::IDataSource>{
			auto ds = sp<Jde::DB::IDataSource>{ GetDataSourceFunction() };
			ds->SetConfig( config );
			return ds;
		}
	};

	flat_map<string,sp<DataSourceApi>> _dataSources; mutex _dsMutex;
	std::once_flag _singleShutdown;
	α DB::DataSource( jobject config, SL sl )ε->sp<IDataSource>{
		fs::path driver{ config.at("driver").as_string().c_str() };
		THROW_IF( !fs::is_regular_file(driver), Exception(sl, {ELogLevel::Critical, ELogTags::App}, "Dynamic Library '{}' not found.", driver.string()) );
		lg _{_dsMutex};
		string key = driver.string();
		auto source = _dataSources.find( key );
		if( source==_dataSources.end() )
			source = _dataSources.emplace( key, ms<DataSourceApi>(move(driver)) ).first;
		return source->second->Emplace( move(config) );
	}

	α DB::GetCluster( sv configName, sp<Access::IAcl> authorize, SL sl )ε->sp<Cluster>{
		for( auto& cluster : getClusters(authorize) )
			if( cluster->ConfigName==configName )
				return cluster;
		THROW( Exception(sl, {ELogLevel::Critical, ELogTags::App}, "Cluster '{}' not found.", configName) );
	}

	α DB::GetAppSchema( str metaName, sp<Access::IAcl> authorize, optional<jobject> dbSettings )ε->sp<AppSchema>{
		for( auto& cluster : getClusters(authorize, dbSettings) ){
			for( auto& catalog : cluster->Catalogs ){
				if( auto schema = catalog->FindAppSchema(metaName); schema )
					return schema;
			}
		}
		THROW( "Schema '{}' not found.", metaName );
	}


	α DB::SyncSchema( const AppSchema& schema, sp<QL::IQL> ql )ε->void{
		SchemaDdl::Sync( schema, ql );
	}
}

#ifndef PROD
namespace Jde::DB{
	bool recreated{}; //TODO: make this per dbSchema.
	α NonProd::Recreate( const AppSchema& schema, sp<QL::IQL> ql )ε->void{
		if( !recreated )
			CatalogDdl::NonProd::Drop( schema );
		recreated = true;
		return SchemaDdl::Sync( schema, ql );
	}
#endif
}