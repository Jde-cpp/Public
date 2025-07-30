{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbDriver: "$(JDE_BUILD_DIR)/msvc/jde/apps/IotWebsocket/bin/RelWithDebInfo/Jde.DB.Odbc.dll",
	dbConnectionString: "DSN=release",
	catalogs: {
		test_opc_debug: {
			schemas:{
				_access:{
					access:{
						meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet"
					}
				},
				opc:{
					opc:{
						meta: "$(JDE_DIR)/IotWebsocket/config/opcGateway-meta.jsonnet",
						prefix: ""  //test with null prefix, debug with prefix
					}
				}
			}
		}
	}
}