#include "globals.h"
#include "jde/fwk/settings.h"
#include "jde/db/meta/AppSchema.h"
#include "jde/db/meta/Cluster.h"
#include "jde/ql/LocalQL.h"
#include <jde/access/Authorize.h>

#define let const auto

namespace Jde::DB::Sqlite{
	flat_map<string, sp<IDataSource>> _dsMap;
	//`cluster` is a dbServers key ("memory" or "file") - GetCluster builds every configured cluster once and returns the named one.
	α Tests::DS( str cluster, bool clear, SL sl )ε->sp<IDataSource>{
		if( _dsMap.contains(cluster) ){
			if( clear )
				_dsMap.erase( cluster );
			else
				return _dsMap.at( move(cluster) );
		}
		auto ds = DB::GetCluster( cluster, ms<Access::Authorize>("SqliteTests"), sl )->DataSource;
		_dsMap[move( cluster )] = ds;
		return ds;
	}
	namespace Tests{
		struct SqliteQL final : QL::LocalQL{
			SqliteQL( vector<sp<DB::AppSchema>> schemas, sp<Access::Authorize> authorizer )ι: LocalQL{ move(schemas), move(authorizer) }{}
			α LogQuery( QL::TableQL&&, SL )ι->up<TAwait<jvalue>> override{ ASSERT(false); return nullptr; }
			α StatusQuery( QL::TableQL&& )ι->jobject override{ ASSERT(false); return {}; }
			α CustomQuery( QL::TableQL&, QL::Creds, SL )ι->up<TAwait<jvalue>> override{ return nullptr; }
			α CustomMutation( QL::MutationQL&, QL::Creds, SL )ι->up<TAwait<jvalue>> override{ ASSERT(false); return nullptr; }
			α LogSettingsQuery( QL::TableQL&&, SL )ι->up<TAwait<jvalue>> override{ ASSERT(false); return nullptr; }
		};

		//A file-backed cluster's db is shared (cached in _clusters) across every fixture, so reset it once per process -
		//before the connection opens - so stale rows from a prior run don't collide. :memory: is empty per-connection.
		α resetFile( str cluster )ε->void{
			static flat_set<string> _reset;
			if( !_reset.insert(cluster).second )
				return;
			let path = Settings::FindString( Ƒ("/dbServers/{}/catalogs/testDb/path", cluster) ).value_or( "" );
			if( path.empty() || path==":memory:" )
				return;
			fs::remove( path );
			auto base = fs::path{ path };
			fs::remove( base.replace_extension(".db-shm") );
			fs::remove( base.replace_extension(".db-wal") );
		}

		α Schema::Create( str cluster )ε->void{
			resetFile( cluster );
			auto authorizer = ms<Access::Authorize>( "SqliteTests" );
			auto dbCluster = DB::GetCluster( cluster, authorizer );
			auto accessSchema = dbCluster->GetAppSchema( "access" );
			_dsMap[cluster] = dbCluster->DataSource;
			auto appSchema = dbCluster->GetAppSchema( "app" );
			auto uaSchema = dbCluster->GetAppSchema( "opc" );
			auto gatewaySchema = dbCluster->GetAppSchema( "gateway" );

			// TODO add fks
			vector<sp<AppSchema>> schemas{ accessSchema, appSchema, uaSchema, gatewaySchema };
			QL::Configure( schemas );
			auto ql = ms<SqliteQL>( move(schemas), move(authorizer) );
			DB::SyncSchema( *accessSchema, ql );
			DB::SyncSchema( *appSchema, ql );
			DB::SyncSchema( *uaSchema, ql );
			DB::SyncSchema( *gatewaySchema, ql );
		}
	}
}