{
  local args = self,
	local buildTarget = std.extVar("buildTarget"),
	local repoBuildDir = "$(REPO_BUILD_DIR)/"+buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	sqlType: "sqlite",
	logsDir: std.extVar("logsDir"),
	dbServers: {
		localhost:{
			driver: repoBuildDir+"/libs/db/drivers/sqlite/lib/libJde.DB.Sqlite.so",
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
								dynamicLib: repoBuildDir+"/apps/AppServer/config/sql/sqlite/libJde.DB.Sqlite.AppServer.so"
							},
						}
					}
				}
			}
		}
	},
}