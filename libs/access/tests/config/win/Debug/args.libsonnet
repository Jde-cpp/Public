{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbServers: {
		localhost:{
			driver: "$(JDE_BUILD_DIR)/msvc/jde/libs/db/drivers/odbc/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=TestAccessDebug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				test_access_debug: {
					schemas:{
						acc:{   //test debug with schema, debug with default schema ie dbo.
							access:{
								meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.jsonnet",
								prefix: null  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	}
}