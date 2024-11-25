{
	testing:{
		testsa:"CertificateTests.*",
		tests:"SocketTests.BadTransmissionClient",
		testsc:"WebTests.EchoAttack"
	},
	cryptoTests:{
		clear: true
	},
	logging:{
		defaultLevel: "Information",
		tags: {
			Trace:["test", "exception", "app",
				"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read"
			],
			Debug:["settings"],
			Information:[],
			Warning:[],
			Error:[],
			Critical:[]
		},
		console:{},
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
			_certificate: "{ApplicationDataFolder}/ssl/certs/server.pem",
			certificateAltName: "DNS:localhost,IP:127.0.0.1",
			_certficateCompany: "Jde-Cpp",
			_certficateCountry: "US",
			_certficateDomain: "localhost",
			_privateKey: "{ApplicationDataFolder}/ssl/private/server.pem",
			_publicKey: "{ApplicationDataFolder}/ssl/public/server.pem",
			_dh: "{ApplicationDataFolder}/certs/dh.pem",
			_passcode: "$(JDE_PASSCODE)"
		}
	},
	workers:{
		executor: 1,
		drive: {threads: 1}
	}
}