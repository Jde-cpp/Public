local args = import 'args.libsonnet';
{
	testing:{
		tests: "PasswordTests.Authenticate",
		recreateDB:: true,
		embeddedAppServer: false,
		embeddedOpcServer: false
	},
	opc: args.opc,
	dbServers: {
		scriptPaths: [
			"$(JDE_DIR)/AppServer/config/sql/"+args.sqlType,
			"$(JDE_DIR)/Public/libs/access/config/sql/"+args.sqlType,
			"$(JDE_DIR)/Public/apps/OpcGateway/config/sql/"+args.sqlType,
			"$(JDE_DIR)/Public/apps/OpcServer/config/sql/"+args.sqlType
		],
		dataPaths: ["$(JDE_DIR)/AppServer/config", "$(JDE_DIR)/Public/libs/access/config"],
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
		app:{ port: 1967 },
		gateway:{ port: 1968 },
		opcServer:{ port: 1970 }
	},
	credentials:{
		name: "IotTests2",
		target: "IotTests2"
	},
	logging:{
		defaultLevel:: "Information",
		tags: {
			trace:["test","sql", "app",
				"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read",
				"ql"],
			debug:["settings", "scheduler", "uaEvent",
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
		},
		memory: true
	},
	workers:{
		executor: 2,
		drive: {threads: 1}
	}
}