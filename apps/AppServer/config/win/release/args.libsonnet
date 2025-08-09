{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbDriver: "$(JDE_BUILD_DIR)/msvc/jde/apps/AppServer/bin/RelWithDebInfo/Jde.DB.Odbc.dll",
	dbConnectionString: "DSN=release",
	catalogs: {
		release: {
			schemas:{
				acc:{
					access:{  //test debug with schema, debug with default schema ie dbo.
						meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
						ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.jsonnet",
						prefix: ""  //test with null prefix, debug with prefix
					}
				},
				log:{
					log:{
						meta: "$(JDE_DIR)/Public/apps/AppServer/config/log-meta.jsonnet",
						prefix: ""  //test with null prefix, debug with prefix
					},
				}
			}
		}
	}
}