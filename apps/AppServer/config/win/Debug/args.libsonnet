{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbServers: {
		dataPaths: ["$(JDE_DIR)/apps/AppServer/config", "$(JDE_DIR)/libs/access/config"],
		scriptPaths:  ["$(JDE_DIR)/apps/AppServer/config/sql/sqlServer", "$(JDE_DIR)/libs/access/config/sql/sqlServer"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/msvc/libs/db/drivers/odbc/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=debug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				debug: {
					schemas:{
						dbo:{
							access:{  //test debug with schema, debug with default schema ie dbo.
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/libs/access/config/access-ql.jsonnet",
								prefix: "access_"  //test with null prefix, debug with prefix
							},
							app:{
								meta: "$(JDE_DIR)/apps/AppServer/config/app-meta.jsonnet",
								prefix: "app_"
							}
						}
					}
				}
			}
		}
	}
}