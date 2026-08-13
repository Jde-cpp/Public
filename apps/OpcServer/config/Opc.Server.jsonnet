local args = import 'args.libsonnet';
local logsDir = args.logsDir;
function( sync=false )
{
	instanceName: args.instanceName,
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
			tags: {
				default: "Information",
				test: "Trace",
				app: "Debug",
				io: "Warning",
				ql: "Debug",
				settings: "Debug",
				"http.client.read": "Debug",
				"http.client.write": "Debug",
				"socket.client.read": "Debug",
				"socket.client.write": "Debug",
				sql: "Information",
				threads: "Warning",
				"opc.access": "Trace",
				uaEvent: "Debug",
				uaNet: "Information",
				uaSession: "Trace",
				uaServer: "Trace",
				uaUser: "Trace",
				uaSecurity: "Trace",
				uaClient: "Trace",
				uaSecure: "Trace"
			},
			sinks:{
				console:{},
				file:{ path: logsDir, md: false }
			}
		}
	},
	credentials:{
		name: "OpcServer.Test."+args.buildTarget,
		target:: "OpcServer"
	},
	web:{
		client:{ ssl:{ caFile: "$(ProgramData)/Jde-Cpp/AppServer/ssl/certs/AppServer.pem" } }//the AppServer is its own root - without an anchor the client rejects localhost:1967's self-signed cert.
	},
	http:{
		host: "localhost",//advertised to the AppServer registry - the frontend fetches this host, and allowOrigin 'sameHost' requires it to match the page's host (localhost:4200).
		port: 1970,
		ssl:{
			certificate:{
				commonName: args.instanceName + ".web"
			}
		}
	},
	opcServer:{
		target: "TestServer",
		resource: args.buildTarget,
		description: "Test OPC",
		mutationsDir:: args.repoSourceDir + "/apps/OpcServer/config/mutations/pumps",
		db: false,
		ssl:{
			certificate: {
				path:: "{ApplicationDataFolder}/ssl/certs/OpcServer.pem",
				subjectAltName: "URI:urn:open62541.server.application",
				company:: "Jde-Cpp",
				country: "US",
			},
			privateKey: {
				path:: "{ApplicationDataFolder}/ssl/private/OpcServer.pem",
				passcode:: "OpcServer"
			},
			publicKey:{
				path:: "{ApplicationDataFolder}/ssl/public/OpcServer.pem"
			},
			dh:: "{ApplicationDataFolder}/ssl/dh.pem",
		},
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
		port: 4840
	},
	//the UA server's trust list.  UAConfig reads /access/trustedCertDirs, not /opcServer/trustedCertDirs - anchoring it
	//under opcServer leaves the server with zero anchors and every secured client rejected BadCertificateUntrusted.
	access:{
		trustedCertDirs: args.access.trustedCertDirs
	},
	workers:{
		executor:{ threads:  2 },
		drive:{ threads:  2 }
	}
}