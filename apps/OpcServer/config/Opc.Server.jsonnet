local args = import 'args.libsonnet';
{
	dbServers:{
		dataPaths: args.dbServers.dataPaths,
		scriptPaths: args.dbServers.scriptPaths,
		sync:: true,
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
			trace:["test","sql",
				"UASession", "UAServer", "UAUser", "UASecurity", "UAEvent", "threads", "UAClient", "UANet", "UASecure"],
			debug:["ql","settings", "app"],
			information:["app", "appServer", "net"],
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
		target:: "OpcServer"
	},
	http:{port: 1970},
	opcServer:{
		target: "TestServer",
		description: "Test OPC",
		configDir: "$(JDE_DIR)/Public/apps/OpcServer/config/mutations/pumps",
		port: 4840,
		ssl:{
			certificate: "/tmp/cert.pem",
			privateKey: {path:"/tmp/private.pem", passcode: ""}
		}
	},
	workers:{
		executor: 2,
		drive:{ threads:  1 },
		alarm:{ threads:  1 }
	}
}