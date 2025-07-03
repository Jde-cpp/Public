local args = import 'args.libsonnet';
{
	logging:{
		defaultLevel: "Information",
		tags: {
			trace:[ "app", "browse", "ql",
				"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read",
				"uaNet", "uaSecure", "uaSession", "uaServer", "uaClient", "uaUser", "uaSecurity", "uaEvent", "uaPubSub", "uaDiscovery"
			],
			debug:["settings"],
			information:[
				"iot.read", "iot.monitoring", "iot.browse", "app.processingLoop", "iot.monitoring.pedantic"
			],
			warning:[],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{},
			file:{ path: args.logDir, md: false }
		}
	},
	dbServers: {
		scriptPaths: args.dbServers.scriptPaths,
		sync: true,
		localhost:{
			driver: args.dbServers.localhost.driver,
			connectionString: args.dbServers.localhost.connectionString,
			username: args.dbServers.localhost.username,
			password: args.dbServers.localhost.password,
			schema: "debug",
			catalogs: args.dbServers.localhost.catalogs
		}
	},
	credentials:{
		name: "OpcGateway",
		target:: "OpcGateway"
	},
	http:{
		address: null,
		port: 1968,
		threads: 1,
		timeout: "PT30M",
		maxLogLength: 255,
		accessControl: {
			allowOrigin: "*",
			allowMethods: "GET, POST, OPTIONS",
			allowHeaders: "Content-Type, Authorization"
		},
		ssl: {
			certificate:: "{ApplicationDataFolder}/ssl/certs/cert.pem",
			certificateAltName: "DNS:localhost,IP:127.0.0.1",
			certficateCompany:: "Jde-Cpp",
			certficateCountry:: "US",
			certficateDomain:: "localhost",
			privateKey:: "{ApplicationDataFolder}/ssl/private/private.pem",
			publicKey:: "{ApplicationDataFolder}/ssl/public/public.pem",
			dh:: "{ApplicationDataFolder}/certs/dh.pem",
			passcode:: "$(JDE_PASSCODE)"
		}
	},
	workers:{
		drive:{ "threads":  1 },
		alarm:{ "threads":  1 },
		executor: 2
	}
}