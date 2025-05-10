{
	testing:{
		tests:: "UAClientTests.Authenticate",
		recreateDB: true
	},
	opc:{
		urn: "urn:JDE-CPP:Kepware.KEPServerEX.V6:UA%20Server",
		url: "opc.tcp://127.0.0.1:49320"
	},
	dbServers: {
		scriptPaths: ["$(JDE_DIR)/IotWebsocket/config/sql/mysql"],
		localhost:{
			driver: "$(JDE_BUILD_DIR)/$(JDE_BUILD_TYPE)/libs/db/drivers/mysql/libJde.DB.MySql.so",
			connectionString: "$(JDE_CONNECTION)",
			catalogs: {
				_appCatalog:{
					schemas:{
						_appSchema:{
							access:{
								meta: "$(JDE_DIR)/Public/libs/access/config/access-meta.jsonnet"
							}
						},
					},
				},
				jde_opc_test: {
					schemas:{
						test_opc:{ //for sqlserver, test with schema, debug with default schema ie dbo.
							opc:{
								meta: "$(JDE_DIR)/IotWebsocket/config/opcGateway-meta.jsonnet",
								prefix: null  //test with null prefix, debug with prefix
							}
						}
					}
				}
			}
		}
	},
	iot: {
		target: "Default"
	},
	http: {
		ssl: {
			certificate:: "{ApplicationDataFolder}/ssl/certs/cert.pem",
			certificateAltName: "DNS:localhost,IP:"
		}
	},
	credentials:{
		name: "IotTests",
		target: "IotTests"
	},
	logging:{
		defaultLevel:: "Information",
		tags: {
			trace:["test",
				"http.client.write", "http.client.read", "http.server.write", "http.server.read", "socket.client.write", "socket.client.read", "socket.server.write", "socket.server.read",
				"ql", "app", "sql"],
			debug:["settings", "scheduler", "uaEvent", "iot.users", "sql",
				"uaNet", "uaSecure", "uaSession", "uaServer", "uaClient", "uaUser", "uaSecurity", "uaEvent", "uaPubSub", "uaDiscovery",
				"monitoring", "browse", "processingLoop", "monitoring.pedantic"],
			information:["threads", "uaSecure"],
			warning:[],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{},
			file:{ path: "/tmp", md: false }
		},
		memory: true
	},
	workers:{
		executor: 1,
		drive: {threads: 1},
		alarm: {threads: 1}
	}
}