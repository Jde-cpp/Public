local args = import 'args.libsonnet';
local logsDir = args.logsDir;
function( sync=false )
{
	local instance = self,
	gateway:{
		pingInterval: "PT30S",
		ttl: "PT2M",
		search:{ //the `search` query's per-connection node-name index (src/NodeIndex.cpp), crawled on the first search and dropped with the client at ttl.
			maxDepth: 12, //levels under Objects
			maxNodes: 25000, //stop crawling (results flagged truncated) beyond this
			limit: 20, //default rows when the query passes no limit
			includeServer: false //index the ns=0 Server object's diagnostics subtree
		},
		issuedCerts: {
			certificate:{
				subjectAltName: "URI:urn:open62541.server.application",
				commonName: args.instanceName,
			}
		}
	},
	logging:{
		breakLevel: "Critical",
		spd:{
			tags: {
				default: "Information",
				app: "Trace",
				browse: "Trace",
				ql: "Trace",
				access: "Trace",
				"opc.access": "Trace",
				test: "Trace",
				externalLogger: "Information",
				"http.client.write": "Debug",
				"http.client.read": "Debug",
				"http.server.write": "Debug",
				"http.server.read": "Debug",
				"socket.client.write": "Debug",
				"socket.client.write.subscription": "Information",
				"socket.client.read": "Debug",
				"socket.client.read.subscription": "Information",
				"socket.server.write": "Debug",
				"socket.server.read": "Debug",
				settings: "Trace",
				uaEvent: "Debug",
				monitoring: "Information",
				processingLoop: "Information",
				opc: "Trace",
				uaNet: "Information",
				uaClient: "Information",
				uaSecure: "Information",
				uaSession: "Information",
				uaServer: "Information",
				uaUser: "Information",
				uaSecurity: "Information",
				uaPubSub: "Information",
				uaDiscovery: "Information"
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
			"introspection/additive.jsonnet",
			"introspection/serverConnection.jsonnet",
			"introspection/search.jsonnet"
		]
	},
	credentials:{
		name: "OpcGateway",
		target:: "OpcGateway"
	},
	web:{
		client:{ ssl:{ caFile: args.certsDir("AppServer")+"/AppServer.pem" } }//the AppServer is its own root - without an anchor the client rejects localhost:1967's self-signed cert.  The stem is App.Server.jsonnet's visible commonName; if that or its `path::` un-hides, this anchor must follow.
	},
	http:{
		address: null,
		host: "localhost",//advertised to the AppServer registry - the frontend fetches this host, and allowOrigin 'sameHost' requires it to match the page's host (localhost:4200).
		port: 1968,
		threads: 1,
		timeout:: "PT30M",
		socketTimeout:: "P1D",
		maxLogLength: 255,
		bodyLimit: 10000,
		accessControl: {
			allowOrigin: "sameHost",
			allowMethods: "GET, POST, OPTIONS",
			allowHeaders: "Content-Type, Authorization"
		},
		ssl: {
			certificate:{
				subjectAltName: "URI:urn:open62541.server.application",
				company:: "Jde-Cpp",
				fileName: args.instanceName + ".web",
				commonName: args.instanceName + ".web.$(HostName)",
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