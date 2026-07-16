local args = import 'args.libsonnet';
local logsDir = args.logsDir;
{
	testing:{
		tests:: "AccessTests.UserAccess",
		recreateDB: true,
		embeddedAppServer: true,
		UANodeSets: "$(UA_NODE_SETS)"
	},
	opc: args.opc,
	opcServer:{
		target: "TestServer",
		resource: "test",
		description: "Test OPC",
		configDir: args.repoSourceDir + "/apps/OpcServer/config/mutations/pumps",
		port: 4840,
		ssl:{
			certificate: args.repoBuildDir + "/OpcServer/ssl/certs/cert.pem",
			privateKey: {path: args.repoBuildDir + "/OpcServer/ssl/private/private.pem", passcode: ""}
		}
	},
	dbServers: {
		scriptPaths: [
			args.repoSourceDir + "/apps/AppServer/config/sql/"+args.sqlType,
			args.repoSourceDir + "/libs/access/config/sql/"+args.sqlType,
			args.repoSourceDir + "/apps/OpcServer/config/sql/"+args.sqlType
		],
		dataPaths: [args.repoSourceDir + "/apps/AppServer/config", args.repoSourceDir + "/libs/access/config"],
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
		breakLevel: "Critical",
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
				file:{ path: logsDir, md: false }
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