local args = import 'args.libsonnet';
{
	testing:{
		tests:: "UAClientTests.Authenticate",
		recreateDB:: true
	},
	opc: args.opc,
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/IotWebsocket/config/sql/"+args.sqlType],
		sync:: true,
		localhost:{
			driver: args.dbDriver,
			connectionString: args.dbConnectionString,
			catalogs: args.catalogs
		}
	},
	iot: {
		target: "Default"
	},
	http: {
		ssl: {
			certificate:: "{ApplicationDataFolder}/ssl/certs/cert.pem",
			certificateAltName: "DNS:localhost,IP:"
		}
	},
	credentials:{
		name: "IotTests",
		target: "IotTests"
	},
	logging:{
		defaultLevel:: "Information",
		tags: {
			trace:["test",
				"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read",
				"ql", "app", "sql"],
			debug:["settings", "scheduler", "uaEvent", "iot.users", "sql",
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
		executor: 1,
		drive: {threads: 1},
		alarm: {threads: 1}
	}
}