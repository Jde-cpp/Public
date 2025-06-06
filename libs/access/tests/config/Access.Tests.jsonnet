local args = import 'args.libsonnet';
{
	testing:{
		tests:: "AuthTests.Login_New",
		recreateDB:: true
	},
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/Public/libs/access/config/sql/"+args.sqlType],
		dataPaths: ["$(JDE_DIR)/Public/libs/access/config"],
		sync: true,
		localhost:{
			driver: args.dbServers.localhost.driver,
			connectionString: args.dbServers.localhost.connectionString,
			username: args.dbServers.localhost.username,
			password: args.dbServers.localhost.password,
			schema: args.dbServers.localhost.schema,
			catalogs: args.dbServers.localhost.catalogs
		}
	},
	logging:{
		tags: {
			trace:["test", "app", "access", "ql", "sql"],
			debug:["settings"],
			information:[],
			warning:[],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{},
			file:{ path: args.logDir, md: false }
		}
	},
	workers:{
		executor: 2,
		drive: {threads: 1}
	}
}