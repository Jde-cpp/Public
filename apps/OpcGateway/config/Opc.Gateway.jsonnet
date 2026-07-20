local args = import 'args.libsonnet';
local logsDir = args.logsDir;
function( sync=false )
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
				externalLogger: "Trace",
				http_client_write: "Debug",
				http_client_read: "Debug",
				http_server_write: "Debug",
				http_server_read: "Debug",
				socket_client_write: "Debug",
				socket_client_read: "Debug",
				socket_server_write: "Debug",
				socket_server_read: "Debug",
				settings: "Trace",
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
				file:{ path: logsDir, md: false }
			}
		},
		proto:{
			path: logsDir + "/opc-gateway",
			timeZone: "America/New_York",
			delay: "PT1M",
			tags: {
				default: "Debug",
				externalLogger: "None"
			}
		}
	},
	dbServers: {
		scriptPaths: args.dbServers.scriptPaths,
		sync: sync,
		localhost:{
			driver: args.dbServers.localhost.driver,
			connectionString: args.dbServers.localhost.connectionString,
			username: args.dbServers.localhost.username,
			password: args.dbServers.localhost.password,
			schema: args.dbServers.localhost.schema,
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
		executor:{ threads:  3 }
	}
}