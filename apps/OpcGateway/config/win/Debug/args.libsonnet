{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	opc:{
		urn: "urn:JDE-CPP:Kepware.KEPServerEX.V6:UA%20Server",
		url: "opc.tcp://127.0.0.1:49320"
	},
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/IotWebsocket/config/sql/sqlServer"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/msvc/jde/libs/db/drivers/odbc/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=debug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				debug: {
					schemas:{
						_access:{
							access:{
								meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet"
							}
						},
						dbo:{
							opc:{
								meta: "$(JDE_DIR)/IotWebsocket/config/opcGateway-meta.jsonnet",
								prefix: "opc"  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	}
}