{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	dbDriver: "$(JDE_BUILD_DIR)/msvc/jde/libs/opc/bin/RelWithDebInfo/Jde.DB.Odbc.dll",
	dbConnectionString: "DSN=OpcTestsRelease",
	opc:{
		urn: "urn:JDE-CPP:Kepware.KEPServerEX.V6:UA%20Server",
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
						meta: "$(JDE_DIR)/IotWebsocket/config/opcGateway-meta.jsonnet",
						prefix: ""  //test with null prefix, debug with prefix
					}
				}
			}
		}
	}
}