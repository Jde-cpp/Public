{
	sqlType: "mysql",
	logDir: "$(JDE_BUILD_DIR)",
	opcServer: {
		trustedCertDirs: [
			"$(HOME)/.Jde-Cpp/OpcGateway/ssl/certs",
			"$(HOME)/.Jde-Cpp/Tests.Opc/ssl/certs"
		],
		ssl:{
			certificate: "/tmp/cert.pem",
			privateKey: {path:"/tmp/private.pem", passcode: ""}
		}
	},
	dbServers: {
		dataPaths: [],
		scriptPaths:  ["$(JDE_DIR)/apps/OpcServer/config/sql/mysql"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "debug",
			catalogs: {
				AppServer: {
					schemas:{
						_appServer:{
							access:{  //test debug with schema, debug with default schema ie dbo.
								meta: "$(JDE_DIR)/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/libs/access/config/access-ql.jsonnet",
								prefix: "access_"  //test with null prefix, debug with prefix
							},
						},
						debug:{
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