local common = import '../../../../../libs/db/config/sqlite-common.libsonnet';
common + {
	local args = self,
	local cwd = std.extVar("cwd"),
	instanceName: args.buildTarget+".sqlite",
	opcServer: {
		trustedCertDirs: [
			common.certsDir( "OpcGateway" ),
			common.certsDir( common.opcTestsProduct )
		],
		ssl:{
			certificate: cwd+"/ssl/certs/OpcServer.pem",
			privateKey: {path: cwd+"/ssl/private/OpcServer.pem", passcode: "OpcServer"}
		}
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
