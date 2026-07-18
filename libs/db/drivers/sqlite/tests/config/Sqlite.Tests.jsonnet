local logsDir = std.extVar("logsDir");
local buildTarget = std.extVar("buildTarget");
local repoBuildDir = "$(REPO_BUILD_DIR)/"+buildTarget;
local repoSourceDir = "$(REPO_SOURCE_DIR)";
local cluster(path) = { //one backend; instantiated per-path as the 'memory' and 'file' clusters below.
	driver: repoBuildDir+"/libs/db/drivers/sqlite/lib/libJde.DB.Sqlite.so",
	catalogs: {
		testDb: { // n/a for sqlite
			path: path,
			schemas:{
				master:{ // n/a for sqlite
					access:{
						meta: repoSourceDir + "/libs/access/config/access-meta.jsonnet",
						ql: repoSourceDir + "/libs/access/config/access-ql.jsonnet",
						prefix: "access_",
						dynamicLib: repoBuildDir+"/apps/AppServer/config/sql/sqlite/libJde.DB.Sqlite.AppServer.so"
					},
					app:{
						meta: repoSourceDir + "/apps/AppServer/config/app-meta.jsonnet",
						prefix: "app_",
						dynamicLib: repoBuildDir+"/apps/AppServer/config/sql/sqlite/libJde.DB.Sqlite.AppServer.so"
					},
					opc:{
						meta: repoSourceDir +"/apps/OpcServer/config/opcServer-meta.jsonnet",
						prefix: "opc_",
						dynamicLib: repoBuildDir+"/apps/OpcServer/config/sql/sqlite/libJde.DB.Sqlite.OpcServer.so"
					},
					gateway:{
						meta: repoSourceDir + "/apps/OpcGateway/config/opcGateway-meta.jsonnet",
						prefix: "gtw_",
						dynamicLib: repoBuildDir+"/apps/OpcGateway/config/sql/sqlite/libJde.DB.Sqlite.OpcGateway.so"
					}
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
