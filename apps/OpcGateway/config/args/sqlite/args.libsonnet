local common = import '../../../../../libs/db/config/sqlite-common.libsonnet';
common + {
	local args = self,
	dbServers: {
		scriptPaths: [args.repoSourceDir + "/apps/OpcGateway/config/sql/"+args.sqlType],
		localhost: common.localhost({
			_access:{
				//meta only: mounted for its type definitions, no prefix and no twins loaded here - the '_' prefix
				//makes SqliteDataSource skip the schema when loading proc dlls.
				access:{ meta: common.accessMeta }
			},
			dbo:{ // n/a for sqlite
				gateway: common.gateway(),
			}
		})
	}
}
