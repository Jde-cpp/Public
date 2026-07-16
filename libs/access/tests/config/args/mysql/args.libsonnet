{
  local args = self,
	local buildTarget = std.extVar("buildTarget"),
	local repoBuildDir = "$(REPO_BUILD_DIR)/"+buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	sqlType: "mysql",
	logsDir: std.extVar("logsDir"),
	dbServers: {
		localhost:{
			driver: repoBuildDir+"/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: buildTarget+"_access",
			catalogs: {
				master: {  // N/A for mysql
					schemas:{
						[buildTarget+"_access"]:{//test debug with schema, debug with default schema ie dbo.
							access:{
								meta: args.repoSourceDir+"/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir+"/libs/access/config/access-ql.jsonnet",
								prefix: "acc_"  //test with null prefix, debug with prefix
							},
						}
					}
				}
			}
		}
	},
}