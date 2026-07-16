{
	local args = self,
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	schema():: if args.buildTarget == "release" then "rls" else args.buildTarget,
	repoBuildDir: "$(REPO_BUILD_DIR)/"+args.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	sqlType: "mysql",
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	dbServers: {
		scriptPaths: [args.repoSourceDir + "/apps/OpcGateway/config/sql/mysql"],
		sync:: true,
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: args.schema(),
			catalogs: {
				IotWebsocket: {
					schemas:{
						_access:{
							access:{
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet"
							}
						},
						[args.schema()]:{
							gateway:{
								meta: args.repoSourceDir + "/apps/OpcGateway/config/opcGateway-meta.jsonnet",
								prefix: "opc_"  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	}
}