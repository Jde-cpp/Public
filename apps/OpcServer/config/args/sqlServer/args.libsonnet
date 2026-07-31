local common = import '../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	sqlType: "sqlServer",
	instanceName: "OpcServer."+args.sqlType+"."+args.buildTarget,
	access: {
		trustedCertDirs: [
			args.certsDir( "OpcGateway" ),
			args.certsDir( "OpcTests" ) //sqlServer is the windows default args dir, so the windows ProductName (OpcTests.rc) - linux uses args/mysql.
		],
	},
	dbServers: {
		dataPaths: [],
		scriptPaths:  [ args.repoSourceDir+"/apps/OpcServer/config/sql/sqlServer"],
		localhost:{
			driver: args.repoBuildDir+"/bin/Jde.DB.Odbc.dll",
			connectionString: "DSN="+args.schema(),
			username: null,
			password: null,
			schema: null,
			catalogs: {
				[args.schema()]: {
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