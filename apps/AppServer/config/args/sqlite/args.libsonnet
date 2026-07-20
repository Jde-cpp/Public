{
  local args = self,
	sqlType: "sqlite",
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+args.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	dbServers: {
		scriptPaths: [
			args.repoSourceDir+"/libs/access/config/sql/sqlite",
			args.repoSourceDir+"/apps/AppServer/config/sql/sqlite",
		],
		dataPaths: [
			args.repoSourceDir+"/apps/AppServer/config",
			args.repoSourceDir+"/libs/access/config"
		],
		localhost:{
			driver: args.repoBuildDir+"/libs/db/drivers/sqlite/lib/libJde.DB.Sqlite.so",
			connectionString: null,
			username: null,
			password: null,
			schema: null,
			catalogs: {
				master: {  // n/a for sqlite
					path: std.extVar("path"),
					schemas:{
						dbo:{ // n/a for sqlite
							access:{
								meta: args.repoSourceDir+"/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir+"/libs/access/config/access-ql.jsonnet",
								prefix: "access_",  //needs to be access_
								dynamicLib: args.repoBuildDir+"/apps/AppServer/config/sql/sqlite/libJde.DB.Sqlite.AppServer.so"
							},
							app:{
								meta: args.repoSourceDir + "/apps/AppServer/config/app-meta.jsonnet",
								prefix: "app_",  //needs to be app_
								dynamicLib: args.repoBuildDir+"/apps/AppServer/config/sql/sqlite/libJde.DB.Sqlite.AppServer.so"
							},
						}
					}
				}
			}
		}
	},
}