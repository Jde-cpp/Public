local args = import 'args.libsonnet';
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
	logging:{
		spd:{
			flushOn: "Trace",
			defaultLevel: "Information",
			tags:{
				trace:["exception","parsing", "test", "sessions",
					"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read"
				],
				debug:["ql","sql", "settings"],
				information:[],
				warning:[],
				"error":[],
				critical:[]
			},
			sinks:{
				console:{},
				file:{ path: args.logDir, md: false }
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