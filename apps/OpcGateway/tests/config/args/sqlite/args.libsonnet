local common = import '../../../../../../libs/db/config/sqlite-common.libsonnet';
common + {
	instanceName: common.buildTarget + "-" + (if common.windows then "windows" else "linux"),
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	access: {
		trustedCertDirs: [
			common.certsDir( common.opcTestsProduct ), //gateway client cert - no ssl productName, so it lands under the test exe's product.
			common.certsDir( "OpcServer" )
		]
	},
	opcServer: {
		trustedCertDirs: if common.windows then [
			common.certsDir( common.opcTestsProduct )
		] else [
			common.certsDir( "OpcGateway" ),
			common.certsDir( common.opcTestsProduct )
		]
	},
	dbServers: {
		localhost: common.localhost({
			dbo:{ // n/a for sqlite
				access: common.access(),
				app: common.app(),
				opc: common.opcSchema(),
				gateway: common.gateway(),
			}
		})
	}
}
