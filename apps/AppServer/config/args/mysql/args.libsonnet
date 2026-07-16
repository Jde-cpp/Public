{
	local args = self,
	sqlType: "mysql",
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	schema():: if args.buildTarget == "release" then "rls" else args.buildTarget,
	repoBuildDir: "$(REPO_BUILD_DIR)/"+args.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	dbServers: {
		dataPaths: [ args.repoSourceDir + "/apps/AppServer/config", args.repoSourceDir + "/libs/access/config"],
		scriptPaths:  [args.repoSourceDir + "/apps/AppServer/config/sql/mysql", args.repoSourceDir + "/libs/access/config/sql/mysql"],
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: args.schema(),
			catalogs: {
				master: { // n/a for mysql
					schemas:{
						[args.schema()]:{
							access:{  //test debug with schema, debug with default schema ie dbo.
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir + "/libs/access/config/access-ql.jsonnet",
								prefix: "access_"  //test with null prefix, debug with prefix
							},
							app:{
								meta: args.repoSourceDir + "/apps/AppServer/config/app-meta.jsonnet",
								prefix: "app_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	}
}