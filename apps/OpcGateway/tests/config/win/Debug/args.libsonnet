{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	opcServer: {
		trustedCertDirs: [
			"$(ProgramData)/Jde-Cpp/OpcTests/ssl/certs"
		]
	},
	dbServers: {
		localhost:{
			driver: "$(JDE_BUILD_DIR)/msvc/Debug/libs/db/drivers/odbc/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=OpcTestsDebug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				test_opc_debug: {
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
						gateway:{
							gateway:{
								meta: "$(JDE_DIR)/apps/OpcGateway/config/opcGateway-meta.jsonnet",
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