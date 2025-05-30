local args = import 'args.libsonnet';
{
	testing:{
		tests:: "UserTests.Fields",
		recreateDB:: true
	},
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/Public/libs/access/config/sql/"+args.sqlType],
		dataPaths: ["$(JDE_DIR)/Public/libs/access/config"],
		sync: true,
		localhost:{
			driver: args.dbDriver,
			connectionString: args.dbConnectionString,
			catalogs: args.catalogs
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
		executer: 1,
		drive: {threads: 1}
	}
}