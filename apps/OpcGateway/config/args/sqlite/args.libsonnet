{
	local args = self,
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+args.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	sqlType: "sqlite",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	dbServers: {
		scriptPaths: [args.repoSourceDir + "/apps/OpcGateway/config/sql/"+args.sqlType],
		sync:: true,
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/sqlite/lib/libJde.DB.Sqlite.so",
			connectionString: null,
			username: null,
			password: null,
			schema: null,
			catalogs: {
				master: { // n/a for sqlite
					schemas:{
						_access:{
							access:{
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet"
							}
						},
						dbo:{ // n/a for sqlite
							gateway:{
								meta: args.repoSourceDir + "/apps/OpcGateway/config/opcGateway-meta.jsonnet",
								prefix: "gateway_",
								dynamicLib: args.repoBuildDir+"/apps/OpcGateway/config/sql/sqlite/libJde.DB.Sqlite.OpcGateway.so"
							}
						}
					}
				}
			}
		}
	}
}