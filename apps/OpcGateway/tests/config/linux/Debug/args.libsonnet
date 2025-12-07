{
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	opcServer: {
		trustedCertDirs: [
			"$(HOME)/.Jde-Cpp/OpcGateway/ssl/certs",
			"$(HOME)/.Jde-Cpp/Tests.Opc/ssl/certs"
		]
	},
	dbServers: {
		localhost:{
			driver: "$(JDE_BUILD_DIR)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "test_opc",
			catalogs: {
				test_opc_debug: {
					schemas:{
						test_opc:{
							access:{
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet"
							},
							log:{
								meta: "$(JDE_DIR)/apps/AppServer/config/log-meta.jsonnet",
								prefix: ""   //test with null prefix, debug with prefix
							},
							gateway:{
								meta: "$(JDE_DIR)/apps/OpcGateway/config/opcGateway-meta.jsonnet",
								prefix: ""
							},
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