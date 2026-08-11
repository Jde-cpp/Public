local common = import '../../../../../../libs/db/config/args-common.libsonnet';
common + {
	local args = self,
	sqlType: "mysql",
	dbServers: {
		localhost:{
			driver: args.repoBuildDir + "/libs/db/drivers/mysql/lib/libJde.DB.MySql.so",
			connectionString: null,
			username: "$(JDE_MYSQL_USER)",
			password: "$(JDE_MYSQL_PWD)",
			schema: "test_app_server",
			catalogs: {
				master:{ // n/a for mysql
					schemas:{
						test_app_server:{
							access:{
								meta: args.repoSourceDir + "/libs/access/config/access-meta.jsonnet",
								ql: args.repoSourceDir + "/libs/access/config/access-ql.jsonnet",
								prefix: null  //test with null prefix, debug with prefix
							},
							app:{
								meta: args.repoSourceDir + "/apps/AppServer/config/app-meta.jsonnet",
								prefix: null
							}
						}
					}
				}
			}
		}
	}
}
