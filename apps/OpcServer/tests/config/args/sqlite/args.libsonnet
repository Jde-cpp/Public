{
	local args = self,
	sqlType: "sqlite",
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+args.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	dbServers: {
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/sqlite/lib/libJde.DB.Sqlite.so",
			connectionString: null,
			username: null,
			password: null,
			schema: null,
			catalogs: {
				master:{ // n/a for sqlite
					schemas:{
						dbo:{ // n/a for sqlite
							access:{
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir + "/libs/access/config/access-ql.jsonnet",
								prefix: "access_",
								dynamicLib: args.repoBuildDir+"/apps/AppServer/config/sql/sqlite/libJde.DB.Sqlite.AppServer.so"
							},
							app:{
								meta: args.repoSourceDir + "/apps/AppServer/config/app-meta.jsonnet",
								prefix: "app_",
								dynamicLib: args.repoBuildDir+"/apps/AppServer/config/sql/sqlite/libJde.DB.Sqlite.AppServer.so"
							},
							opc:{
								meta: args.repoSourceDir + "/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: "opc_",
								dynamicLib: args.repoBuildDir+"/apps/OpcServer/config/sql/sqlite/libJde.DB.Sqlite.OpcServer.so"
							},
						}
					}
				}
			}
		}
	}
}