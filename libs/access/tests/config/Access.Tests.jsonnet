local args = import 'args.libsonnet';
{
	testing:{
		tests:: "GroupTests.Fields",
		recreateDB: true
	},
	dbServers:{
		scriptPaths: ["$(JDE_DIR)/libs/access/config/sql/"+args.sqlType],
		dataPaths: ["$(JDE_DIR)/libs/access/config"],
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
		spd:{
			tags: {
				trace:["test", "access", "app"],
				debug:["settings", "sql", "ql"],
				information:["app"],
				warning:[],
				"error":[],
				critical:[]
			},
			sinks:{
				console:{},
				file:{ path: args.logDir, md: false }
			}
		}
	},
	workers:{
		executor: {threads: 2}
	}
}