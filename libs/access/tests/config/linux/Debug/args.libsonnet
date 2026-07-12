{
  local args = self, // captures the parent object scope
	repoBuildDir: "$(REPO_BUILD_DIR)",
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	sqlType: "mysql",
	dbServers: {
		localhost:{
			driver: args.repoBuildDir+"/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "test_access",
			catalogs: {
				test_access_debug: {
					schemas:{
						test_access:{//test debug with schema, debug with default schema ie dbo.
							access:{
								meta: args.repoSourceDir+"/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir+"/libs/access/config/access-ql.jsonnet",
								prefix: null  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	}
}