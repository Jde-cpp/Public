local common = import '../../../../../libs/db/config/args-common.libsonnet';
//As args/mysql, for the windows default.  gateway prefix "opc_" as mysql - the gateway's own sqlServer args say "opc"
//(no underscore); a box provisioned by the split gateway may need that spelling instead.
common + {
	local args = self,
	sqlType: "sqlServer",
	dbServers: {
		dataPaths: [ args.repoSourceDir + "/apps/AppServer/config", args.repoSourceDir + "/libs/access/config" ],
		scriptPaths: [
			args.repoSourceDir + "/libs/access/config/sql/sqlServer",
			args.repoSourceDir + "/apps/AppServer/config/sql/sqlServer",
			args.repoSourceDir + "/apps/OpcGateway/config/sql/sqlServer"
		],
		localhost:{
			driver: args.repoBuildDir+"/bin/Jde.DB.Odbc.dll",
			connectionString: "DSN="+args.schema(),
			username: null,
			password: null,
			schema: null,
			catalogs: {
				[args.schema()]: {
					schemas:{
						dbo:{
							access:{
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir + "/libs/access/config/access-ql.jsonnet",
								prefix: "access_"
							},
							app:{
								meta: args.repoSourceDir + "/apps/AppServer/config/app-meta.jsonnet",
								prefix: "app_"
							},
							gateway:{
								meta: args.repoSourceDir + "/apps/OpcGateway/config/opcGateway-meta.jsonnet",
								prefix: "opc_"
							}
						}
					}
				}
			}
		}
	}
}
