local logsDir = std.extVar("logsDir");
{
	testing:{
		tests:: "SocketTests.*",
		certDir: logsDir+'/web-tests/ssl'
	},
	logging:{
		spd:{
			tags:{
				default: "Information",
				test: "Trace",
				exception: "Trace",
				app: "Trace",
				http_client_sessions: "Trace",
				http_client_write: "Trace",
				http_client_read: "Trace",
				http_server_write: "Trace",
				http_server_read: "Trace",
				socket_client_write: "Trace",
				socket_client_read: "Trace",
				socket_server_write: "Trace",
				socket_server_read: "Trace",
				settings: "Debug"
			},
			sinks:{
				console:{},
				file:{ path: logsDir, md: false }
			}
		},
		memory:{
			tags:{
				default: "Trace",
				locks: "Warning"
			}
		}
	},
	http:{
		address: null,
		port: 5005,
		# Rest session-expiry duration. WebTests.TestTimeout sleeps timeout+1s, so keep it short.
		timeout: "PT3S",
		# Websocket session-expiry duration.  A session is promoted to this the moment a socket
		# connects on it (Sessions.cpp UpdateExpiration), and SocketTests.SocketPromotesSessionTimeout
		# asserts the jump - so keep it far enough above timeout to be unambiguous.
		socketTimeout: "PT30S",
		maxLogLength: 31,
		accessControl: {
			allowOrigin: "*",
			allowMethods: "GET, POST, OPTIONS",
			allowHeaders: "Content-Type, Authorization"
		},
		ssl: {
			certificate:: "{ApplicationDataFolder}/ssl/certs/server.pem",
			certificateAltName: "DNS:localhost,IP:127.0.0.1",
			certficateCompany:: "Jde-Cpp",
			certficateCountry:: "US",
			certficateDomain:: "localhost",
			privateKey:: "{ApplicationDataFolder}/ssl/private/server.pem",
			publicKey:: "{ApplicationDataFolder}/ssl/public/server.pem",
			dh:: "{ApplicationDataFolder}/certs/dh.pem",
			passcode:: "$(JDE_PASSCODE)"
		}
	},
	workers:{
		executor: {threads: 2},
		drive: {threads: 1}
	}
}