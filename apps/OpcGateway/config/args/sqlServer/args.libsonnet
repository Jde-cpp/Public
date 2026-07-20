{
	sqlType: "sqlServer",
	logsDir: std.extVar("logsDir"),
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/apps/OpcGateway/config/sql/sqlServer"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/$(JDE_COMPILER)/bin/Debug/Jde.DB.Odbc.dll",
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