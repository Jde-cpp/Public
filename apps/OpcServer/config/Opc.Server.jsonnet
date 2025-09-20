local args = import 'args.libsonnet';
{
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
			tags: {
				trace:["test","sql",
					"uaSession", "uaServer", "uaUser", "uaSecurity", "threads", "uaClient", "uaSecure"],
				debug:["ql","settings", "app", "uaEvent", "uaNet"],
				information:[],
				warning:["io"],
				"error":[],
				critical:[]
			},
			sinks:{
				console:{},
				file:{ path: args.logDir, md: false }
			}
		}
	},
	credentials:{
		name: "OpcServer.Test.$(JDE_BUILD_TYPE)",
		target:: "OpcServer"
	},
	http:{port: 1970},
	opcServer:{
		target: "TestServer",
		description: "Test OPC",
		mutationsDir:: "$(JDE_DIR)/Public/apps/OpcServer/config/mutations/pumps",
		db: false,
		configFile: "$(UA_NODE_SETS)/CommercialKitchenEquipment/Opc.Ua.CommercialKitchenEquipment.NodeSet2.xml",
		trustedCertDirs: args.opcServer.trustedCertDirs,
		port: 4840,
		ssl:{
			certificate: "/tmp/cert.pem",
			privateKey: {path:"/tmp/private.pem", passcode: ""}
		}
	},
	workers:{
		executor: 2,
		drive:{ threads:  1 },
		alarm:{ threads:  1 }
	}
}