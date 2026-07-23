local common = import '../../../../config/sqlite-common.libsonnet';
local logsDir = common.logsDir;
local repoSourceDir = common.repoSourceDir;
local lib = common.lib;
local cluster(path) = { //one backend; instantiated per-path as the 'memory' and 'file' clusters below.
	driver: lib( "Jde.DB.Sqlite", "/libs/db/drivers/sqlite/lib" ),
	catalogs: {
		testDb: { // n/a for sqlite
			path: path,
			schemas:{
				master:{ // n/a for sqlite
					access: common.access(),
					app: common.app(),
					opc: common.opcSchema(),
					gateway: common.gateway( {prefix: "gtw_"} )
				}
			}
		}
	}
};
{
	local args = self,
	testing:{
		tests:: "*/SchemaTests.*/file"
	},
	instanceName: "SqliteTests",
	dbServers:{//clusters
		scriptPaths: [
			repoSourceDir+"/libs/access/config/sql/sqlite",
			repoSourceDir+"/apps/AppServer/config/sql/sqlite",
			repoSourceDir+"/apps/OpcServer/config/sql/sqlite"
		],
		dataPaths: [
			repoSourceDir+"/apps/AppServer/config",
			repoSourceDir+"/libs/access/config"
		],
		sync:: true,
		memory: cluster(":memory:"),
		file: cluster( std.extVar("cwd")+"/sqlite-tests.db" )
	},
	logging:{
		spd:{
			tags: {
				default: "Information",
				app: "Trace",
				exception: "Trace",
				test: "Trace",
				settings: "Debug",
				sql: "Debug"
			},
			sinks:{
				console:{},
				file:{ path: logsDir, md: false }
			}
		}
	},
	workers:{
		executor: {threads: 2}
	}
}
