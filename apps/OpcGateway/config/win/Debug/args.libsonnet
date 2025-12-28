{
	sqlType: "sqlServer",
	logDir: "$(JDE_BUILD_DIR)",
	opc:{
		urn: "urn:JDE-CPP:Kepware.KEPServerEX.V6:UA%20Server",
		url: "opc.tcp://127.0.0.1:49320"
	},
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/apps/OpcGateway/config/sql/sqlServer"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/msvc/apps/OpcGateway/exe/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=debug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				debug: {
					schemas:{
						_access:{
							access:{
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet"
							}
						},
						dbo:{
							gateway:{
								meta: "$(JDE_DIR)/apps/OpcGateway/config/opcGateway-meta.jsonnet",
								prefix: "opc"  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	}
}