local args = import 'args.libsonnet';
local logsDir = args.logsDir;
function( sync=false )
{
	http:{
		address: null,
		port: 1967,
		threads: 1,
		timeout: "P1D",
		socketTimeout:: "P1D",
		maxLogLength: 255,
		bodyLimit: 10000,
		accessControl:{
			allowOrigin: "sameHost",
			allowMethods: "GET, POST, OPTIONS",
			allowHeaders: "Content-Type, Authorization"
		},
		clientSettings:{
			googleAuthClientId:"445012155442-1v8ntaa22konm0boge6hj5mfs15o9lvd.apps.googleusercontent.com"
		},
		ssl:{
			certificate:{
				path:: "{ApplicationDataFolder}/ssl/certs/AppServer.http.pem",
				subjectAltName: "DNS:localhost,IP:127.0.0.1",
				company:: "Jde-Cpp",
				country: "US",
				commonName: "AppServer"
			},
			privateKey:{
				path:: "{ApplicationDataFolder}/ssl/private/AppServer.http.pem",
				passcode:: "$(JDE_PASSCODE)"
			},
			publicKey:{
				path:: "{ApplicationDataFolder}/ssl/public/AppServer.http.pem"
			},
			dh:{
				path:: "{ApplicationDataFolder}/ssl/dh.pem"
			},
		},
	},
	access:{
		//operator drop-dir for enrollment trust anchors: a client cert (.pem/.crt) copied here authorizes its key-login enrollment. Rescanned on failed verification - no restart needed.
		//Production products only.  Every cert under these dirs can enroll a user whose identity is the cert's CN, so a
		//test/dev product dir here would let anything that writes one provision an account in the production access db;
		//the test binaries anchor their own dirs in their own configs (Opc.Server.Tests.jsonnet, Opc.Tests.jsonnet).
		trustedCertDirs: [
			"$(ProgramData)/Jde-Cpp/OpcServer/ssl/certs",
			"$(ProgramData)/Jde-Cpp/OpcGateway/ssl/certs"
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
				http_client_write: "Debug",
				http_client_read: "Debug",
				http_server_write: "Debug",
				http_server_read: "Debug",
				socket_client_write: "Debug",
				socket_client_read: "Debug",
				socket_server_write: "Debug",
				socket_server_read: "Debug",
				socket_client_read_subscription: "Information",
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