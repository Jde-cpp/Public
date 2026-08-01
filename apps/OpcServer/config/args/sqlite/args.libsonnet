local common = import '../../../../../libs/db/config/sqlite-common.libsonnet';
common + {
	local args = self,
	local cwd = std.extVar("cwd"),
	instanceName: "OpcServer."+args.sqlType+"."+args.buildTarget,
	access: {
		trustedCertDirs: [
			common.certsDir( "OpcGateway" ),
			common.certsDir( common.opcTestsProduct )
		]
	},
	dbServers: {
		dataPaths: [],
		scriptPaths:  [args.repoSourceDir + "/apps/OpcServer/config/sql/"+args.sqlType],
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
