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
				trace:["test","sql"],
				debug:["ql","settings", "app"],
				information:[],
				warning:["io","uaEvent", "uaNet", "uaSession",
					"uaServer", "uaUser", "uaSecurity", "threads", "uaClient", "uaSecure"],
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
		mutationsDir:: "$(JDE_DIR)/apps/OpcServer/config/mutations/pumps",
		db: false,
		opcNodeSet:{
			path: "$(UA_NODE_SETS)/Opc.Ua.PredefinedNodes.xml",
			nodeIds: [23513]
		},
		machinery: [
			//"$(JDE_DIR)/apps/OpcServer/config/nodesets/uaPredefinedNodes.xml",
			//"/home/duffyj/Downloads/Opc.Ua.NodeSet2.xml",
			"$(UA_NODE_SETS)/DI/Opc.Ua.Di.NodeSet2.xml",
			"$(UA_NODE_SETS)/IA/Opc.Ua.IA.NodeSet2.xml",
			"$(UA_NODE_SETS)/Machinery/Opc.Ua.Machinery.NodeSet2.xml",
			"$(UA_NODE_SETS)/Machinery/Opc.Ua.Machinery.Examples.NodeSet2.xml",
		],
		additive: [
//			"$(JDE_DIR)/apps/OpcServer/config/nodesets/uaPredefinedNodes.xml",
			"$(UA_NODE_SETS)/DI/Opc.Ua.Di.NodeSet2.xml",
			"$(UA_NODE_SETS)/Machinery/Opc.Ua.Machinery.NodeSet2.xml",
			"$(UA_NODE_SETS)/ISA95-JOBCONTROL/opc.ua.isa95-jobcontrol.nodeset2.xml",
			//"$(UA_NODE_SETS)/Machinery/ProcessValues/Opc.Ua.Machinery.ProcessValues.NodeSet2.xml",
			"$(UA_NODE_SETS)/PADIM/Opc.Ua.IRDI.NodeSet2.xml",
			"$(UA_NODE_SETS)/PADIM/Opc.Ua.PADIM.NodeSet2.xml",
			"$(UA_NODE_SETS)/IA/Opc.Ua.IA.NodeSet2.xml",
			"$(UA_NODE_SETS)/Machinery/Jobs/Opc.Ua.Machinery.Jobs.Nodeset2.xml",
			"$(UA_NODE_SETS)/MachineTool/Opc.Ua.MachineTool.NodeSet2.xml",
			"$(UA_NODE_SETS)/AdditiveManufacturing/Opc.Ua.AdditiveManufacturing.Nodeset2.xml",
			"$(UA_NODE_SETS)/AdditiveManufacturing/AdditiveManufacturing-Example.xml",
		],
		configFiles: [
			//"$(JDE_DIR)/apps/OpcServer/config/nodesets/uaPredefinedNodes.xml",
			"$(UA_NODE_SETS)/DI/Opc.Ua.Di.NodeSet2.xml",
			"$(UA_NODE_SETS)/IA/Opc.Ua.IA.NodeSet2.xml",
			"$(UA_NODE_SETS)/IA/Opc.Ua.IA.NodeSet2.examples.xml"
		],
		trustedCertDirs: args.opcServer.trustedCertDirs,
		port: 4840,
		ssl:{
			certificate: args.opcServer.ssl.certificate,
			privateKey: {path: args.opcServer.ssl.privateKey.path, passcode: args.opcServer.ssl.privateKey.passcode}
		}
	},
	workers:{
		executor:{ threads:  2 },
		drive:{ threads:  2 }
	}
}