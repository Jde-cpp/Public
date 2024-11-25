{
	testing:{
		tests:: "RoleTests.AddRemove",
		recreateDB:: true
	},
	dbServers: {
		scriptPath: "$(JDE_DIR)/Public/libs/access/config/sql",
		localhost:{
			driver: "$(JDE_DIR)/bin/asan/libJde.MySql.so",
			connectionString: "$(JDE_CONNECTION)",
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