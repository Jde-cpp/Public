local common = import '../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	local cwd = std.extVar("cwd"),
	sqlType: "mysql",
	instanceName: "OpcServer."+args.sqlType+"."+args.buildTarget,
	access: {
		trustedCertDirs: [
			//Production products only - never a test product's dir.  Every cert under these dirs opens a secured UA session,
			//and wherever this list is also the enrollment anchor (the test hosts, Jde.Opc.Hub) enrolls a user named by its
			//CN; the test binaries anchor their own dirs in their own configs (Opc.Server.Tests.jsonnet, Opc.Tests.jsonnet).
			args.certsDir( "OpcGateway" ),
			args.certsDir( "OpcHub" ), //Jde.Opc.Hub - the gateway role's OPC client certs live under its own product dir.
			args.certsDir( "PlcEmulator" ) //apps/OpcServer/emulator - its UA client cert.
		]
	},
	dbServers: {
		dataPaths: [],
		scriptPaths: [],
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/mysql/lib/libJde.DB.MySql.so",
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