{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbDriver: "$(JDE_BUILD_DIR)/msvc/jde/libs/opc/bin/RelWithDebInfo/Jde.DB.Odbc.dll",
	dbConnectionString: "DSN=OpcTestsRelease",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:49320"
	},
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
						meta: "$(JDE_DIR)/Public/apps/OpcGateway/config/opcGateway-meta.jsonnet",
						prefix: ""  //test with null prefix, debug with prefix
					}
				}
			}
		}
	}
}