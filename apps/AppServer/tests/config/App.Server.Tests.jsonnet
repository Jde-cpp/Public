local args = import 'args.libsonnet';
local logsDir = args.logsDir;
{
	testing:{
		tests:: "*",
		recreateDB: true
	},
	dbServers: {
		scriptPaths: [
			args.repoSourceDir + "/apps/AppServer/config/sql/"+args.sqlType,
			args.repoSourceDir + "/libs/access/config/sql/"+args.sqlType
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
		//1972: unique to this suite - 1967 collides with a running Jde.AppServer, 1968/1970 with the gateway/opcServer suites.
		app:{ port: 1972, ssl:{ certificate:{ subjectAltName: "DNS:localhost,IP:127.0.0.1", commonName: args.instanceName + ".appServer.tests.web" } } },//TLS clients match the SAN - without it host_name_verification rejects the generated cert.
		clientSettings:{
			googleAuthClientId: "app-server-tests-google-client-id"//served by GET /GoogleAuthClientId and ql setting(target:"googleAuthClientId").
		}
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
				socket_server_read: "Debug"
			},
			sinks:{
				console:{},
				file:{ path: logsDir, md: false }
			}
		},
		memory:{
			tags:{ default: "Trace" }//Init() reads /logging/memory/tags - a bare `default` silently drops the memory logger and Logging::Find/ClearMemory throw.
		}
	},
	workers:{
		executor: {threads: 2}
	}
}
