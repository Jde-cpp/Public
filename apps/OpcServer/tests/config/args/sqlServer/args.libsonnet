{
	local args = self,
	sqlType: "sqlServer",
	buildTarget: std.extVar("buildTarget"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+args.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	logsDir: std.extVar("logsDir"),
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	dbServers: {
		localhost:{
			driver: "$(JDE_BUILD_DIR)/$(JDE_COMPILER)/bin/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=OpcServerTestsDebug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				test_opc_server_debug: {
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
						},
						opc:{
							opc:{
								meta: "$(JDE_DIR)/apps/OpcServer/config/opcServer-meta.jsonnet",
							},
						}
					}
				}
			}
		}
	}
}