{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	dbServers: {
		localhost:{
			driver: "$(JDE_BUILD_DIR)/msvc/jde/libs/db/drivers/odbc/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=OpcServerTestsDebug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				test_opc_server_debug: {
					schemas:{
						access:{
							access:{
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet"
							}
						},
						log:{
							log:{
								meta: "$(JDE_DIR)/apps/AppServer/config/log-meta.jsonnet",
							}
						},
						opc:{
							opc:{
								meta: "$(JDE_DIR)/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: ""
							},
						}
					}
				}
			}
		}
	}
}