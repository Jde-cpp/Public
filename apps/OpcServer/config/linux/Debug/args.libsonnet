{
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	opcServer: {
		trustedCertDirs: [
			"$(HOME)/.Jde-Cpp/OpcGateway/ssl/certs"
		]
	},
	dbServers: {
		dataPaths: [],
		scriptPaths:  ["$(JDE_DIR)/Public/apps/OpcServer/config/sql/mysql"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/jde/$(JDE_BUILD_TYPE)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "debug",
			catalogs: {
				AppServer: {
					schemas:{
						_appServer:{
							access:{  //test debug with schema, debug with default schema ie dbo.
								meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.jsonnet",
								prefix: "access_"  //test with null prefix, debug with prefix
							},
						},
						debug:{
							opc:{
								meta: "$(JDE_DIR)/Public/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: "opc_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	}
}