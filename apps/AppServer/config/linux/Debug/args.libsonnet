{
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	dbServers: {
		dataPaths: ["$(JDE_DIR)/AppServer/config", "$(JDE_DIR)/Public/libs/access/config"],
		scriptPaths:  ["$(JDE_DIR)/AppServer/config/sql/mysql", "$(JDE_DIR)/Public/libs/access/config/sql/mysql"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/jde/$(JDE_BUILD_TYPE)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "debug",
			catalogs: {
				AppServer: {
					schemas:{
						debug:{
							access:{  //test debug with schema, debug with default schema ie dbo.
								meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.jsonnet",
								prefix: "access_"  //test with null prefix, debug with prefix
							},
							log:{
								meta: "$(JDE_DIR)/AppServer/config/log-meta.jsonnet",
								prefix: "log_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	}
}