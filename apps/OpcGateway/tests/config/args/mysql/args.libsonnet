local common = import '../../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	sqlType: "mysql",
	instanceName: "debug-linux",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	opcServer: {
		trustedCertDirs: [
			"$(HOME)/.Jde-Cpp/OpcGateway/ssl/certs",
			"$(HOME)/.Jde-Cpp/Tests.Opc/ssl/certs"
		]
	},
	dbServers: {
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "test_opc",
			catalogs: {
				test_opc_debug: {
					schemas:{
						test_opc:{
							access:{
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet"
							},
							app:{
								meta: args.repoSourceDir + "/apps/AppServer/config/app-meta.jsonnet",
								prefix: ""   //test with null prefix, debug with prefix
							},
							gateway:{
								meta: args.repoSourceDir + "/apps/OpcGateway/config/opcGateway-meta.jsonnet",
								prefix: ""
							},
							opc:{
								meta: args.repoSourceDir + "/apps/OpcServer/config/opcServer-meta.jsonnet",
								prefix: ""
							},
						}
					}
				}
			}
		}
	}
}