#include <jde/db/db.h>
#include <jde/framework/settings.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/AppSchema.h>
#include "meta/ddl/CatalogDdl.h"
#include "meta/ddl/SchemaDdl.h"
#include "meta/IServerMeta.h"
#include <jde/framework/Dll.h>
#include "c_api.h"

#define let const auto

namespace Jde::DB{
	vector<sp<Cluster>> _clusters;
	fs::path _scriptPath;
	α GetClusters( sp<Access::IAcl> authorize )ε->vector<sp<Cluster>>{
		if( _clusters.empty() ){
			let& clusters = Settings::AsObject( "/dbServers" );
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
		DataSourceApi( const fs::path& path ):
			_dll{ path },
			GetDataSourceFunction{ _dll["GetDataSource"] }
		{}
		decltype(GetDataSource) *GetDataSourceFunction;

		α Emplace( str connectionString )->sp<DB::IDataSource>{
			std::unique_lock l{ _connectionsMutex };
			auto pDataSource = _connections.find( connectionString );
			if( pDataSource == _connections.end() ){
				auto pNew = sp<Jde::DB::IDataSource>{ GetDataSourceFunction() };
				pNew->SetConnectionString( connectionString );
				pDataSource = _connections.emplace( connectionString, pNew ).first;
			}
			return pDataSource->second;
		}
		static flat_map<string,sp<Jde::DB::IDataSource>> _connections; static mutex _connectionsMutex;
	};
	flat_map<string,sp<DB::IDataSource>> DataSourceApi::_connections; mutex DataSourceApi::_connectionsMutex;

	flat_map<string,sp<DataSourceApi>> _dataSources; mutex _dsMutex;
	std::once_flag _singleShutdown;
	α DB::DataSource( const fs::path& libraryName, sv connectionString )ε->sp<IDataSource>{
		THROW_IF( !fs::is_regular_file(libraryName), "Library '{}' not found.", libraryName.string() );
		THROW_IF( connectionString.empty(), "Connection string is empty." );
		sp<IDataSource> pDataSource;
		std::unique_lock l{_dsMutex};
		string key = libraryName.string();
		auto pSource = _dataSources.find( key );
		if( pSource==_dataSources.end() )
			pSource = _dataSources.emplace( key, ms<DataSourceApi>(libraryName) ).first;
		return pSource->second->Emplace( string{connectionString} );
	}
	α DB::GetAppSchema( str metaName, sp<Access::IAcl> authorize )ε->sp<AppSchema>{
		for( auto& cluster : GetClusters(authorize) ){
			for( auto& catalog : cluster->Catalogs ){
				if( auto pSchema = catalog->FindAppSchema(metaName); pSchema )
					return pSchema;
			}
		}
		THROW( "Schema '{}' not found.", metaName );
	}


	α DB::SyncSchema( AppSchema& schema, sp<QL::IQL> ql )ε->void{
		SchemaDdl::Sync( schema, ql );
	}
}

#ifndef PROD
namespace Jde::DB{
	α NonProd::Recreate( const AppSchema& schema, sp<QL::IQL> ql )ε->void{
		CatalogDdl::NonProd::Drop( schema );
		return SchemaDdl::Sync( schema, ql );
	}
#endif
}