{
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	dbServers: {
		localhost:{
			driver: "$(JDE_DIR)/Release/g++-14/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "test_accessr",
			catalogs: {
				test_access_release: {
					schemas:{
						test_accessr:{//test debug with schema, debug with default schema ie dbo.
							access:{
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/libs/access/config/access-ql.jsonnet",
								prefix: "acc_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	},
}