local args = import 'args.libsonnet';
{
	testing:{
		tests: "WebTests.*"
	},
	logging:{
		defaultLevel:: "Information",
		tags: {
			trace:["test", "exception", "app", "http.client.sessions",
				"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read"
			],
			debug:["settings"],
			information:[],
			warning:[],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{},
			file:{ path: args.logDir, md: false }
		},
		memory: true
	},
	http:{
		address: null,
		port: 5005,
		timeout: "PT10S",
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
		executor: 1,
		drive: {threads: 1}
	}
}