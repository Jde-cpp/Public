local args = import 'args.libsonnet';
{
	gateway:{
		pingInterval: "PT30S",
		ttl: "PT2M"
	},
	logging:{
		spd:{
			defaultLevel:: "Information",
			tags: {
				trace:[ "app", "browse", "ql", "access", "opc.access", "test",
					"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read",
					"uaSecure","uaSession", "uaServer", "uaUser", "uaSecurity", "uaPubSub", "uaDiscovery"
				],
				debug:["settings", "uaEvent"],
				information:[
					"uaNet","uaClient",
					"opc.read", "opc.monitoring", "opc.browse", "app.processingLoop", "opc.monitoring.pedantic"
				],
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
	dbServers: {
		scriptPaths: args.dbServers.scriptPaths,
		sync:: true,
		localhost:{
			driver: args.dbServers.localhost.driver,
			connectionString: args.dbServers.localhost.connectionString,
			username: args.dbServers.localhost.username,
			password: args.dbServers.localhost.password,
			schema: "debug",
			catalogs: args.dbServers.localhost.catalogs
		}
	},
	ql:{
		introspection: [
			"introspection/di.jsonnet",
			"introspection/ia.jsonnet",
			"introspection/machineTool.jsonnet",
			"introspection/additive.jsonnet"
		]
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
			certificateAltName:: "DNS:localhost,IP:127.0.0.1",
			certficateCompany:: "Jde-Cpp",
			certficateCountry:: "US",
			certficateDomain:: "localhost",
			privateKey:: "{ApplicationDataFolder}/ssl/private/private.pem",
			publicKey:: "{ApplicationDataFolder}/ssl/public/public.pem",
			dh:: "{ApplicationDataFolder}/certs/dh.pem",
			passcode:: "$(JDE_PASSCODE)"
		},
		clientSettings:{
			googleAuthClientId: "445012155442-1v8ntaa22konm0boge6hj5mfs15o9lvd.apps.googleusercontent.com"
		}
	},
	workers:{
		drive:{ threads:  2 },
		executor:{ threads:  2 }
	}
}