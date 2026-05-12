local args = import 'args.libsonnet';
{
	instanceName: args.instanceName,
	testing:{
		tests:: "LogSettingTests.UpdateDefault",
		recreateDB:: true,
		embeddedAppServer: false,
		embeddedOpcServer: false
	},
	opc: args.opc,
	dbServers: {
		scriptPaths: [
			"$(JDE_DIR)/apps/AppServer/config/sql/"+args.sqlType,
			"$(JDE_DIR)/libs/access/config/sql/"+args.sqlType,
			"$(JDE_DIR)/apps/OpcGateway/config/sql/"+args.sqlType,
			"$(JDE_DIR)/apps/OpcServer/config/sql/"+args.sqlType
		],
		dataPaths: ["$(JDE_DIR)/apps/AppServer/config", "$(JDE_DIR)/libs/access/config"],
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
		gateway:{ port: 1968, ssl:{ cert:{altName: "URI:urn:open62541.server.application", domain: "localhost"}} },
		opcServer:{ port: 1970, ssl:{productName: "OpcServer"} }
	},
	opcServer:{
		target: "TestServer",
		description: "Test OPC",
		mutationsDir:: "$(JDE_DIR)/apps/OpcServer/config/mutations/pumps",
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
			tags: {
				default: "Information",
				app: "Trace",
				ql: "Trace",
				settings: "Trace",
				scheduler: "Debug",
				http_client_write: "Trace",
				http_client_read: "Trace",
				http_server_write: "Debug",
				http_server_read: "Debug",
				locks: "Information",
				socket_client_read: "Trace",
				socket_client_write: "Trace",
				socket_server_read: "Debug",
				socket_server_write: "Debug",
				test: "Trace",
				threads: "Information",
				processingLoop: "Information",
				sql: "Information",
				browse: "Information",
				monitoring: "Information",
				uaClient: "Information",
				uaDiscovery: "Information",
				uaEvent: "Debug",
				uaNet: "Information",
				uaPubSub: "Information",
				uaSecure: "Information",
				uaSecurity: "Information",
				uaSession: "Information",
				uaServer: "Information",
				uaUser: "Information",
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
		subscribe:{},
		remote:{
			delay: "PT0.001S"
		}
	},
	workers:{
		executor: {threads: 2},
		drive: {threads: 1}
	}
}