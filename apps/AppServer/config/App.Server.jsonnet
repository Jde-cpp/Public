local args = import 'args.libsonnet';
local logsDir = args.logsDir;
function( sync=false )
{
	http:{
		address: null,
		port: 1967,
		threads: 1,
		timeout:: "PT30M",
		socketTimeout:: "P1D",
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
			certificate:{
				path:: "{ApplicationDataFolder}/ssl/certs/AppServer.pem",
				subjectAltName: "DNS:localhost,IP:127.0.0.1",
				company:: "Jde-Cpp",
				country: "US",
				commonName: "AppServer"//subject CN - TLS clients match the SAN, not this; a non-"localhost" value keeps it distinct per product.
			},
			privateKey:{
				path:: "{ApplicationDataFolder}/ssl/private/AppServer.pem",
				passcode:: "$(JDE_PASSCODE)"
			},
			publicKey:{
				path:: "{ApplicationDataFolder}/ssl/public/AppServer.pem"
			},
			dh:{
				path:: "{ApplicationDataFolder}/ssl/dh.pem"
			},
		},
	},
	access:{
		//operator drop-dir for enrollment trust anchors: a client cert (.pem/.crt) copied here authorizes its key-login enrollment. Rescanned on failed verification - no restart needed.
		trustedCertDirs: [
			"$(ProgramData)/jde-cpp/OpcServer/ssl/certs",
			"$(ProgramData)/jde-cpp/OpcGateway/ssl/certs"
		]
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
				sql: "Information",
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