local common = import '../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	sqlType: "mysql",
	instanceName: args.buildTarget,
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
		scriptPaths:  [args.repoSourceDir + "/apps/OpcServer/config/sql/mysql"],
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: args.schema(),
			catalogs: {
				master: { // n/a for mysql
					schemas:{
						_appServer:{
							access:{  //test debug with schema, debug with default schema ie dbo.
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir + "/libs/access/config/access-ql.jsonnet",
								prefix: "access_"  //test with null prefix, debug with prefix
							},
						},
						[args.schema()]:{
							opc:{
								meta: args.repoSourceDir + "/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: "opc_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	}
}