{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbServers: {
		dataPaths: ["$(JDE_DIR)/AppServer/config", "$(JDE_DIR)/Public/libs/access/config"],
		scriptPaths:  ["$(JDE_DIR)/AppServer/config/sql/sqlServer", "$(JDE_DIR)/Public/libs/access/config/sql/sqlServer"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/msvc/jde/libs/db/drivers/odbc/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=debug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				debug: {
					schemas:{
						dbo:{
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