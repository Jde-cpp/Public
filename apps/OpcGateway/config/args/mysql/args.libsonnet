local common = import '../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	sqlType: "mysql",
	dbServers: {
		scriptPaths: [args.repoSourceDir + "/apps/OpcGateway/config/sql/mysql"],
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: args.schema(),
			catalogs: {
				IotWebsocket: {
					schemas:{
						_access:{
							access:{
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet"
							}
						},
						[args.schema()]:{
							gateway:{
								meta: args.repoSourceDir + "/apps/OpcGateway/config/opcGateway-meta.jsonnet",
								prefix: "opc_"  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	}
}