{
	testing:{
		tests: "AclTests.EnabledPermissions",
		recreateDB:: true,
	},
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/Public/libs/access/config/sql/mysql"],
		dataPaths: ["$(JDE_DIR)/Public/libs/access/config"],
		sync:: true,
		localhost:{
			driver: "db/drivers/mysql/libJde.DB.MySql.so",
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
			trace:["test", "ql", "sql", "access"],
			debug:["settings", "app"],
			information:[],
			warning:[],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{}
		}
	},
	workers:{
		executer: 1,
		drive: {threads: 1}
	}
}