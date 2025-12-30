{
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	dbServers: {
		localhost:{
			driver: "$(JDE_BUILD_DIR)/jde/$(JDE_BUILD_TYPE)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "test_opc_server",
			catalogs: {
				test_opc_server_debug:{
					schemas:{
						test_opc_server:{
							access:{
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet"
								ql: "$(JDE_DIR)/libs/access/config/access-ql.jsonnet",
								prefix: null  //test with null prefix, debug with prefix
							},
							app:{
								meta: "$(JDE_DIR)/apps/AppServer/config/app-meta.jsonnet",
								prefix: null
							},
							opc:{
								meta: "$(JDE_DIR)/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: null
							},
						}
					}
				}
			}
		}
	}
}