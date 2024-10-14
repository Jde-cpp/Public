{

	testing:{
		testsx: "UserTests.*",
		recreateDB: true
	},
	dbServers: {
		local:{
			driver: "$(JDE_DB_DRIVER)",
			connectionString: "$(JDE_CONNECTION)",
			catalogs: {
				jde_test: {
					schemas:{
						test_access:{ //for sqlserver, test with schema, run with default schema ie dbo.
							access:{
								meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet",
								ql: "$(JDE_DIR)/Public/libs/access/config/access-ql.json"
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
			Trace:["test", "ql"],
			Debug:["settings", "app"],
			Information:[],
			Warning:[],
			Error:[],
			Critical:[]
		},
		sinks:{
			console:{}
		}
	},
	workers:{
		drive: {threads: 1}
	}
}