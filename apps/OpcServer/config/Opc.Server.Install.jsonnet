// Opc.Server.jsonnet as installed by apps/OpcHub/setup (`-settings=<this file> -include=args/install`).  The base config is
// dev-shaped wherever it reaches outside the product dir - nodesets under $(UA_NODE_SETS) and the repo, a build-target
// resource and credentials name, the split AppServer's cert as the login TLS anchor - and each of those is replaced from
// the install args here; everything else (ports 1970/4840, the registry it logs in to on 1967, pubsub) is unchanged.  An
// overlay like Opc.Server.Hub.jsonnet: jsonnet is lazy, so the base's args.buildTarget/args.repoSourceDir reads are never
// forced once shadowed, and args/install need not define them.
local args = import 'args.libsonnet';
local base = import 'Opc.Server.jsonnet';
function( sync=false ) base( sync ) + {
	opcServer+: {
		resource: "install",
		configFiles: [
			args.nodesetsDir+"/Opc.Ua.Di.NodeSet2.xml", //OPC Foundation UA-Nodeset DI/IA - bundled by the installer.
			args.nodesetsDir+"/Opc.Ua.IA.NodeSet2.xml",
			args.nodesetsDir+"/Opc.Ua.IA.NodeSet2.examples.xml",
			args.nodesetsDir+"/pumps.NodeSet2.xml" //the PLC emulator's tags - urn:jde:pumps
		]
	},
	credentials+: { name: "OpcServer" }, //the login name; the hub enrolls it through its trustedCertDirs anchor on certsDir("OpcServer").
	web+:{ client+:{ ssl+:{ caFile: args.certsDir("OpcHub")+"/OpcHub.pem" } } } //the hub is the registry - Opc.Hub.jsonnet's /http commonName.
}
