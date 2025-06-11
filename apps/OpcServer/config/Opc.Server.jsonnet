local args = import 'args.libsonnet';
{
	dbServers:{
		dataPaths: args.dbServers.dataPaths,
		scriptPaths: args.dbServers.scriptPaths,
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
		flushOn: "Trace",
		tags: {
			trace:["test", "sql"],
			debug:["settings", "app"],
			information:["app", "appServer", "net",
				"UASession", "UAServer", "UAUser", "UASecurity", "UAEvent", "sql", "threads", "UAClient", "UANet", "UASecure"],
			warning:["alarm", "ql", "io", "locks", "settings"],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{}
		}
	},
	credentials:{
		name: "OpcServer.Test.$(JDE_BUILD_TYPE)",
		target:: "OpcGateway"
	},
	opcServer:{
		name: "TestServer",
		description: "Test OPC",
	},
	tcp:{
		port:  4840,
		certificate: "/tmp/cert.pem",
		privateKey: {path:"/tmp/private.pem", passcode: ""}
	},
	workers:{
		executor: 1,
		drive:{ threads:  1 },
		alarm:{ threads:  1 }
	}
}