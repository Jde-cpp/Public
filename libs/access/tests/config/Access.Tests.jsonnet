{
	testing:{
		tests:: "AuthTests.*",
		recreateDB:: true
	},
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/Public/libs/access/config/sql/mysql"],
		dataPaths: ["$(JDE_DIR)/Public/libs/access/config"],
		sync:: true,
		localhost:{
			driver: "$(JDE_BUILD_DIR)/$(JDE_BUILD_TYPE)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: "mysqlx://$(JDE_MYSQL_CREDS)@127.0.0.1:33060/test_access?ssl-mode=disabled",
			catalogs: {
				jde_test: {
					schemas:{
						test_access:{ //for sqlserver, test with schema, debug with default schema ie dbo.
							access:{
								meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.jsonnet",
								prefix: null  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	},
	logging:{
		tags: {
			trace:["test", "access"],
			debug:["settings", "app", "ql", "sql"],
			information:[],
			warning:[],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{},
			file:{ path: "/tmp", md: false }
		}
	},
	workers:{
		executer: 1,
		drive: {threads: 1}
	}
}