{
	local args = self,
	local cwd = std.extVar("cwd"),
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+args.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	sqlType: "sqlite",
	instanceName: args.buildTarget+".sqlite",
	opcServer: {
		trustedCertDirs: [
			"$(HOME)/.Jde-Cpp/OpcGateway/ssl/certs",
			"$(HOME)/.Jde-Cpp/Tests.Opc/ssl/certs"
		],
		ssl:{
			certificate: cwd+"ssl/certs/OpcServer.pem",
			privateKey: {path: cwd+"ssl/private/OpcServer.pem", passcode: "OpcServer"}
		}
	},
	dbServers: {
		dataPaths: [],
		scriptPaths:  [args.repoSourceDir + "/apps/OpcServer/config/sql/"+args.sqlType],
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/sqlite/lib/libJde.DB.Sqlite.so",
			connectionString: null,
			username: null,
			password: null,
			schema: null,
			catalogs: {
				master: { // n/a for sqlite
					schemas:{
						_appServer:{
							access:{  //test debug with schema, debug with default schema ie dbo.
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir + "/libs/access/config/access-ql.jsonnet",
								prefix: "access_"
							},
						},
						dbo:{ // n/a for sqlite
							opc:{
								meta: args.repoSourceDir + "/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: "opc_",
								dynamicLib: args.repoBuildDir+"/apps/OpcServer/config/sql/sqlite/libJde.DB.Sqlite.OpcServer.so"
							},
						}
					}
				}
			}
		}
	}
}