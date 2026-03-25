local args = import 'args.libsonnet';
{
	gateway:{
		pingInterval: "PT30S",
		ttl: "PT2M"
	},
	logging:{
		spd:{
			tags: {
				default: "Information",
				app: "Trace",
				browse: "Trace",
				ql: "Trace",
				access: "Trace",
				opc_access: "Trace",
				test: "Trace",
				http_client_write: "Trace",
				http_client_read: "Trace",
				http_server_write: "Trace",
				http_server_read: "Trace",
				socket_client_write: "Trace",
				socket_client_read: "Trace",
				socket_server_write: "Trace",
				socket_server_read: "Trace",
				settings: "Debug",
				uaEvent: "Debug",
				monitoring: "Information",
				processingLoop: "Information",
				uaNet: "Warning",
				uaClient: "Warning",
				uaSecure: "Warning",
				uaSession: "Warning",
				uaServer: "Warning",
				uaUser: "Warning",
				uaSecurity: "Warning",
				uaPubSub: "Warning",
				uaDiscovery: "Warning"
			},
			sinks:{
				console:{},
				file:{ path: args.logDir, md: false }
			}
		},
		proto:{
			path: args.logDir + "/proto",
			timeZone: "America/New_York",
			delay: "PT1M"
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
			cert:{
				file:: "{AppDataFolder}/ssl/certs/cert.pem",
				altName: "URI:urn:open62541.server.application",
				company:: "Jde-Cpp",
				domain: "localhost",
				country:: "US",
			},
			privateKey:: "{AppDataFolder}/ssl/private/private.pem",
			publicKey:: "{AppDataFolder}/ssl/public/public.pem",
			dh:: "{AppDataFolder}/certs/dh.pem",
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