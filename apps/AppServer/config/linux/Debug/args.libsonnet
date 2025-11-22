{
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	dbServers: {
		dataPaths: ["$(JDE_DIR)/apps/AppServer/config", "$(JDE_DIR)/libs/access/config"],
		scriptPaths:  ["$(JDE_DIR)/apps/AppServer/config/sql/mysql", "$(JDE_DIR)/libs/access/config/sql/mysql"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/$(JDE_BUILD_TYPE)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "debug",
			catalogs: {
				AppServer: {
					schemas:{
						debug:{
							access:{  //test debug with schema, debug with default schema ie dbo.
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/libs/access/config/access-ql.jsonnet",
								prefix: "access_"  //test with null prefix, debug with prefix
							},
							log:{
								meta: "$(JDE_DIR)/apps/AppServer/config/log-meta.jsonnet",
								prefix: "log_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	}
}