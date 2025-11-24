{
	local programDataCompany = self.programDataCompany,
	local programDataApp = self.programDataApp,
	programDataCompany: "$(ProgramData)/jde-cpp",
	programDataApp: programDataCompany+"/OpcServer",
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	opcServer: {
		trustedCertDirs: [
			programDataCompany+"/OpcGateway/ssl/certs",
			programDataCompany+"/OpcTests/ssl/certs"
		],
		ssl:{
			certificate: programDataApp+"/ssl/certs/opcServer.pem",
			privateKey: {path: programDataApp+"/ssl/private/opcServer.pem", passcode: ""}
		}
	},
	dbServers: {
		dataPaths: [],
		scriptPaths:  ["$(JDE_DIR)/apps/OpcServer/config/sql/sqlServer"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/msvc/jde/apps/OpcServer/bin/Debug/Jde.DB.Odbc.dll",
			connectionString: "DSN=debug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				debug: {
					schemas:{
						_appServer:{
							access:{
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/libs/access/config/access-ql.jsonnet",
							},
						},
						dbo:{
							opc:{
								meta: "$(JDE_DIR)/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: "opc_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	}
}