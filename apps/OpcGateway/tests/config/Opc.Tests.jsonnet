local args = import 'args.libsonnet';
{
	testing:{
		tests: "SubscribeTests.Basic",
		recreateDB:: true,
		embeddedAppServer: false,
		embeddedOpcServer: false
	},
	opc: args.opc,
	dbServers: {
		scriptPaths: [
			"$(JDE_DIR)/Public/apps/AppServer/config/sql/"+args.sqlType,
			"$(JDE_DIR)/Public/libs/access/config/sql/"+args.sqlType,
			"$(JDE_DIR)/Public/apps/OpcGateway/config/sql/"+args.sqlType,
			"$(JDE_DIR)/Public/apps/OpcServer/config/sql/"+args.sqlType
		],
		dataPaths: ["$(JDE_DIR)/Public/apps/AppServer/config", "$(JDE_DIR)/Public/libs/access/config"],
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
	iot: {
		target: "Default"
	},
	http:{
		app:{ port: 1967, ssl:{productName: "AppServer"} },
		gateway:{ port: 1968 },
		opcServer:{ port: 1970, ssl:{productName: "OpcServer"} }
	},
	opcServer:{
		target: "TestServer",
		description: "Test OPC",
		mutationsDir:: "$(JDE_DIR)/Public/apps/OpcServer/config/mutations/pumps",
		configFiles: [
			"$(UA_NODE_SETS)/DI/Opc.Ua.Di.NodeSet2.xml",
			"$(UA_NODE_SETS)/IA/Opc.Ua.IA.NodeSet2.xml",
			"$(UA_NODE_SETS)/IA/Opc.Ua.IA.NodeSet2.examples.xml"
		],
		trustedCertDirs: args.opcServer.trustedCertDirs,
		port: 4840,
		ssl:{
			certificate: "$(JDE_BUILD_DIR)/OpcServer/ssl/certs/cert.pem",
			privateKey: {path:"$(JDE_BUILD_DIR)/OpcServer/ssl/private/private.pem", passcode: ""}
		}
	},
	credentials:{
		gateway:{ name: "GatewayTests" },
		opcServer:{ name: "OpcTests" }
	},
	logging:{
		spd:{
			defaultLevel:: "Information",
			tags: {
				trace:["test", "app", "http.client.write", "http.client.read", "ql", "processingLoop"],
				debug:["settings", "sql", "scheduler", "uaEvent",
					"http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read",
					"uaSession", "uaServer", "uaUser", "uaSecurity", "uaEvent", "uaPubSub", "uaDiscovery",
					"monitoring", "browse", "monitoring.pedantic"],
				information:["threads",
					"uaSecure", "uaClient", "uaNet"],
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
		},
		proto:{
			path: args.logDir + "/proto",
			timeZone: "America/New_York",
			delay: "PT1M"
		},
		// remote:{
		// 	delay: "PT2S"
		// }
	},
	workers:{
		executor: {threads: 2},
		drive: {threads: 1}
	}
}