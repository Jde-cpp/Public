local args = import 'args.libsonnet';
{
	testing:{
		tests: "BrowseTests.NodeId",
		recreateDB:: true,
		embeddedAppServer: true,
		embeddedOpcServer: true
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
		configDir: "$(JDE_DIR)/Public/apps/OpcServer/config/mutations/pumps",
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
				trace:["test", "app", "http.client.write", "http.client.read"],
				debug:["settings", "scheduler", "uaEvent","sql", "ql",
					"http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read",
					"uaNet", "uaSecure", "uaSession", "uaServer", "uaClient", "uaUser", "uaSecurity", "uaEvent", "uaPubSub", "uaDiscovery",
					"monitoring", "browse", "processingLoop", "monitoring.pedantic"],
				information:["threads", "uaSecure"],
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
		executor: 2,
		drive: {threads: 1}
	}
}