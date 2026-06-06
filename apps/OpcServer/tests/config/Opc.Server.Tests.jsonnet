local args = import 'args.libsonnet';
{
	testing:{
		tests: "AccessTests.Query",
		recreateDB:: true,
		embeddedAppServer: false,
		UANodeSets: "$(UA_NODE_SETS)"
	},
	opc: args.opc,
	opcServer:{
		target: "TestServer",
		resource: "test",
		description: "Test OPC",
		configDir: "$(JDE_DIR)/apps/OpcServer/config/mutations/pumps",
		port: 4840,
		ssl:{
			certificate: "$(JDE_BUILD_DIR)/OpcServer/ssl/certs/cert.pem",
			privateKey: {path:"$(JDE_BUILD_DIR)/OpcServer/ssl/private/private.pem", passcode: ""}
		}
	},
	dbServers: {
		scriptPaths: [
			"$(JDE_DIR)/apps/AppServer/config/sql/"+args.sqlType,
			"$(JDE_DIR)/libs/access/config/sql/"+args.sqlType,
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
	http:{
		app:{ port: 1967, ssl:{productName: "AppServer"} },
		opcServer:{ port: 1970, ssl:{productName: "OpcServer"} }
	},
	credentials:{
		opcServer:{ name: "OpcTests" }
	},
	logging:{
		spd:{
			tags: {
				default: "Information",
				test: "Trace",
				app: "Trace",
				ql: "Debug",
				settings: "Debug",
				scheduler: "Debug",
				sql: "Debug",
				threads: "Information",
				http_client_write: "Trace",
				http_client_read: "Trace",
				http_server_write: "Debug",
				http_server_read: "Debug",
				socket_client_write: "Debug",
				socket_client_read: "Debug",
				socket_server_write: "Debug",
				socket_server_read: "Debug",
				monitoring: "Debug",
				browse: "Debug",
				processingLoop: "Debug",
				monitoring_pedantic: "Debug",
				uaClient: "Information",
				uaDiscovery: "Information",
				uaEvent: "Debug",
				uaNet: "Information",
				uaPubSub: "Information",
				uaSecure: "Information",
				uaSecurity: "Information",
				uaSession: "Information",
				uaServer: "Information",
				uaUser: "Information"
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
		executor: {threads: 2},
	}
}