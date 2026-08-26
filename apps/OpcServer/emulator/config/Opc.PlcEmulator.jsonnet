// PLC emulator - no database.  Logs into the AppServer (:1967) for a session (its id is the OPC UA issued token), runs its
// own headless UA server holding the pumps nodeset, PUBLISHES the pump process values over OPC UA PubSub into the
// OpcServer (:4840 tcp / the contract's udp url), and keeps a client session on the OpcServer for the run commands the
// UI writes (-transport=write routes every tag over that session instead and skips the PLC server).
//
// First run: -createCert, then -grant once OpcServer has booted, then restart OpcServer (it loads acls at startup).
// Launch with -tests from $buildDir/runtime; -url= -transport= -period= -statusPeriod= -duration= -opcSchema= beat the settings.
local logsDir = std.extVar("logsDir");
local buildTarget = std.extVar("buildTarget");
local pubsub = import '../../config/pubsub/pumps.libsonnet';
// the OpcServer's applicationUri, as the gateway advertises to it (UAClient::Configuration): the client cert's SAN and both
// client uris derive from it - the endpoint filter matches the server's uri, the SAN must match what we advertise.
local certificateUri = "urn:open62541.server.application";
{
	server:{ host: "localhost", port: 1967, isSsl: false },
	credentials:{ name: "PlcEmulator" }, //the AppServer user -grant writes the acl for.
	http:{ ssl:{ productName: "PlcEmulator" } }, //AppServer login cert - keys under $(companyDir)/PlcEmulator/ssl.
	//the AppServer is its own root; without this anchor the login TLS handshake rejects its self-signed cert (as the OpcServer/gateway configs anchor it).
	web:{ client:{ ssl:{ caFile: "$(ProgramData)/Jde-Cpp/AppServer/ssl/certs/AppServer.pem" } } },
	emulator:{
		transport: "pubsub", //pubsub | write
		url: "opc.tcp://127.0.0.1:4840",
		certificateUri: certificateUri,
		opcSchema: "opc."+buildTarget, ///opcServer/resource = args.buildTarget -> the acl schema.
		plc:{
			port: 4841, //the emulated PLC's own UA endpoint - headless, but a UA_Server must bind something.
			nodeset: "$(JDE_DIR)/apps/OpcServer/config/nodesets/pumps.NodeSet2.xml" //the same file the OpcServer loads.
		},
		pubsub: pubsub,
		period: "PT1S",
		statusPeriod: "PT1M",
		reconnectMin: "PT1S",
		reconnectMax: "PT30S",
		ssl:{ //the UA channel cert; same product dir as the login cert, so one trustedCertDirs entry covers it.
			productName: "PlcEmulator",
			certificate:{ commonName: "PlcEmulator.opc", subjectAltName: "URI:"+certificateUri, country: "US" }
		},
		//path = browse path under Objects; tag names are browse names in the same namespace.  A tag named in the pubsub
		//contract ("<device>.<tag>") is published; command tags are subscribed; everything else is written over the session.
		devices:[
			{ path: "pumps~pump1", tags:[ { name: "motorRpm", mode: "follow", ratedRpm: 1450, tau: "PT3S" }, { name: "status", mode: "command" } ] },
			{ path: "pumps~pump2", tags:[ { name: "motorRpm", mode: "sine", min: 800, max: 1600, period: "PT30S" }, { name: "status", mode: "toggle", period: "PT15S" } ] },
			{ path: "pumps~pump3", tags:[ { name: "motorRpm", mode: "ramp", min: 0, max: 1450, period: "PT20S" }, { name: "status", mode: "command" } ] },
			{ path: "pumps~pump4", tags:[ { name: "motorRpm", mode: "randomWalk", min: 1300, max: 1500, step: 10 }, { name: "status", mode: "command" } ] },
			{ path: "pumps~pumpManual", tags:[ { name: "motorRpm", mode: "counter", min: 0, max: 100000, step: 1 } ] }
		]
	},
	logging:{
		spd:{
			tags:{ default: "Information", uaNet: "Warning", uaClient: "Warning", uaSecure: "Warning", uaSession: "Warning", uaServer: "Warning", uaPubSub: "Information" },
			sinks:{ console:{}, file:{ path: logsDir, md: false } }
		}
	},
	workers:{ drive:{ threads: 1 }, executor:{ threads: 2 } }
}
