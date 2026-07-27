local common = import '../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	sqlType: "sqlServer",
	dbServers: {
		scriptPaths: [args.repoSourceDir+"/apps/OpcGateway/config/sql/sqlServer"],
		localhost:{
			driver: args.repoBuildDir+"/bin/Jde.DB.Odbc.dll",
			connectionString: "DSN=debug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				[args.schema()]: {
					schemas:{
						_access:{
							access:{
								meta: args.repoSourceDir+"/libs/access/config/access-meta.jsonnet"
							}
						},
						dbo:{
							gateway:{
								meta: args.repoSourceDir+"/apps/OpcGateway/config/opcGateway-meta.jsonnet",
								prefix: "opc"  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	}
}