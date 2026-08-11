local common = import '../../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	sqlType: "sqlServer",
	dbServers: {
		localhost:{
			driver: args.repoBuildDir+"/bin/Jde.DB.Odbc.dll",
			connectionString: "DSN=AppServerTestsDebug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				test_app_server_debug: {
					schemas:{
						acc:{
							access:{
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet"
							}
						},
						app:{
							app:{
								meta: "$(JDE_DIR)/apps/AppServer/config/app-meta.jsonnet",
							}
						}
					}
				}
			}
		}
	}
}
