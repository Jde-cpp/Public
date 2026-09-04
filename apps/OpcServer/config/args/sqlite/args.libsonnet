local common = import '../../../../../libs/db/config/sqlite-common.libsonnet';
common + {
	local args = self,
	local cwd = std.extVar("cwd"),
	instanceName: "OpcServer."+args.sqlType+"."+args.buildTarget,
	access: {
		trustedCertDirs: [
			common.certsDir( "OpcGateway" ),
			common.certsDir( "OpcHub" ), //Jde.Opc.Hub - the gateway role's OPC client certs live under its own product dir.
			common.certsDir( common.opcTestsProduct ),
			common.certsDir( "PlcEmulator" ) //apps/OpcServer/emulator - its UA client cert.
		]
	},
	dbServers: {
		dataPaths: [],
		scriptPaths: [],
		localhost: common.localhost({
			_appServer:{
				//test debug with schema, debug with default schema ie dbo.  No dynamicLib: this process loads no
				//access twins - the '_' prefix makes SqliteDataSource skip the schema when loading proc dlls.
				access:{ meta: common.accessMeta, ql: common.accessQL, prefix: "access_" },
			},
			dbo:{ // n/a for sqlite
				opc: common.opcSchema(),
			}
		})
	}
}
