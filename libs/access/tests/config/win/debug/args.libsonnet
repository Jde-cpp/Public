{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbDriver: "$(JDE_BUILD_DIR)/msvc/jde/libs/access/bin/debug/Jde.DB.Odbc.dll",
	dbConnectionString: "DSN=AccessTestDebug",
	catalogs: {
		jde_test_access_debug: {
			schemas:{
				acc:{
					access:{  //test debug with schema, debug with default schema ie dbo.
						meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
						ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.jsonnet",
						prefix: null  //test with null prefix, debug with prefix
					},
				}
			}
		}
	}
}