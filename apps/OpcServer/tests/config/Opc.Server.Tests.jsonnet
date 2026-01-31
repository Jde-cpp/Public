local args = import 'args.libsonnet';
{
	testing:{
		tests: "AccessTests.UserAccess",
		recreateDB: true,
		embeddedAppServer:: false,
		UANodeSets: "$(UA_NODE_SETS)"
	},
	opc: args.opc,
	opcServer:{
		target: "TestServer",
		resource: "test",
		description: "Test OPC",
		configDir: "$(JDE_DIR)/apps/OpcServer/config/mutations/pumps",
		port: 4840,
		ssl:{
			certificate: "$(JDE_BUILD_DIR)/OpcServer/ssl/certs/cert.pem",
			privateKey: {path:"$(JDE_BUILD_DIR)/OpcServer/ssl/private/private.pem", passcode: ""}
		}
	},
	dbServers: {
		scriptPaths: [
			"$(JDE_DIR)/apps/AppServer/config/sql/"+args.sqlType,
			"$(JDE_DIR)/libs/access/config/sql/"+args.sqlType,
			"$(JDE_DIR)/apps/OpcServer/config/sql/"+args.sqlType
		],
		dataPaths: ["$(JDE_DIR)/apps/AppServer/config", "$(JDE_DIR)/libs/access/config"],
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
	http:{
		app:{ port: 1967, ssl:{productName: "AppServer"} },
		opcServer:{ port: 1970, ssl:{productName: "OpcServer"} }
	},
	credentials:{
		opcServer:{ name: "OpcTests" }
	},
	logging:{
		spd:{
			defaultLevel:: "Information",
			tags: {
				trace:["test", "app", "http.client.write", "http.client.read"],
				debug:["settings", "scheduler", "uaEvent","sql", "ql",
					"http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read",
					"monitoring", "browse", "processingLoop", "monitoring.pedantic"],
				information:["threads", "uaSecure",
				"uaNet", "uaSecure", "uaSession", "uaServer", "uaClient", "uaUser", "uaSecurity", "uaEvent", "uaPubSub", "uaDiscovery"],
				warning:[],
				"error":[],
				critical:[]
			},
			sinks:{
				console:{},
				file:{ path: args.logDir, md: false }
			}
		},
		memory:{
			default: "trace"
		}
	},
	workers:{
		executor: {threads: 2},
	}
}