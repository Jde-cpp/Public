local args = import 'args.libsonnet';
local logsDir = args.logsDir;
function( sync=false )
{
	http:{
		address: null,
		port: 1967,
		threads: 1,
		timeout: "PT30M",
		maxLogLength: 255,
		accessControl:{
			allowOrigin: "*",
			allowMethods: "GET, POST, OPTIONS",
			allowHeaders: "Content-Type, Authorization"
		},
		clientSettings:{
			googleAuthClientId:"445012155442-1v8ntaa22konm0boge6hj5mfs15o9lvd.apps.googleusercontent.com"
		},
		ssl:{
			_certificate: "{ApplicationDataFolder}/ssl/certs/cert.pem",
			certificateAltName: "DNS:localhost,IP:127.0.0.1",
			_certficateCompany: "Jde-Cpp",
			_certficateCountry: "US",
			_certficateDomain: "localhost",
			_privateKey: "{ApplicationDataFolder}/ssl/private/private.pem",
			_publicKey: "{ApplicationDataFolder}/ssl/public/public.pem",
			_dh: "{ApplicationDataFolder}/certs/dh.pem",
			_passcode: "$(JDE_PASSCODE)"
		},
	},
	dbServers:{
		dataPaths: args.dbServers.dataPaths,
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
	logging:{
		spd:{
			flushOn: "Trace",
			tags:{
				default: "Information",
				sql: "Trace",
				exception: "Debug",
				parsing: "Trace",
				test: "Trace",
				sessions: "Trace",
				http_client_write: "Trace",
				http_client_read: "Trace",
				http_server_write: "Trace",
				http_server_read: "Trace",
				socket_client_write: "Trace",
				socket_client_read: "Trace",
				socket_server_write: "Trace",
				socket_server_read: "Trace",
				ql: "Debug",
				settings: "Debug"
			},
			sinks:{
				console:{},
				file:{ path: logsDir, md: false }
			}
		},
		subscribe:{},
		breakLevel: "Warning"
	},
	workers:{
		drive:{threads: 1},
		executor: {threads: 2},
	}
}