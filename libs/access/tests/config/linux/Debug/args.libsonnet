{
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	dbServers: {
		localhost:{
			driver: "$(JDE_BUILD_DIR)/jde/$(JDE_BUILD_TYPE)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "test_access",
			catalogs: {
				test_access_debug: {
					schemas:{
						test_access:{//test debug with schema, debug with default schema ie dbo.
							access:{
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/libs/access/config/access-ql.jsonnet",
								prefix: null  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	},
}