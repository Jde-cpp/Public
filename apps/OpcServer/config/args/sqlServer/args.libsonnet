local common = import '../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	programDataCompany: "$(ProgramData)/jde-cpp",
	programDataApp: args.programDataCompany+"/OpcServer",
	sqlType: "sqlServer",
	instanceName: args.buildTarget,
	opcServer: {
		trustedCertDirs: [
			args.programDataCompany+"/OpcGateway/ssl/certs",
			args.programDataCompany+"/OpcTests/ssl/certs"
		],
		ssl:{
			certificate: args.programDataApp+"/ssl/certs/opcServer.pem",
			privateKey: {path: args.programDataApp+"/ssl/private/opcServer.pem", passcode: ""}
		}
	},
	dbServers: {
		dataPaths: [],
		scriptPaths:  [ args.repoSourceDir+"/apps/OpcServer/config/sql/sqlServer"],
		localhost:{
			driver: args.repoBuildDir+"/bin/Jde.DB.Odbc.dll",
			connectionString: "DSN=debug",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				debug: {
					schemas:{
						_appServer:{
							access:{
								meta: args.repoSourceDir+"/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir+"/libs/access/config/access-ql.jsonnet",
							},
						},
						dbo:{
							opc:{
								meta: args.repoSourceDir+"/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: "opc_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	}
}