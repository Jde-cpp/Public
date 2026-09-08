local paths = import '../../../../../libs/db/config/paths-common.libsonnet';
//The installed layout (apps/OpcHub/setup): one product dir holding the AppServer's and the gateway's meta/sql, one sqlite
//file beside them.  paths-common only - the service starts without -tests, so no ext vars, which is why sqlite-common
//(buildTarget/logsDir/windows/path) is not imported and its localhost/schema shapes are spelled out here instead.
//`$(ExeDir)` is the dir of the running exe (settings.cpp builtIns): the driver and the proc MODULEs ship beside it,
//wherever the installer put it - Program Files, or a per-user Programs dir - so no root is hardcoded.  The SQL Server
//variant is args/install-sqlServer.
paths + {
	local args = self,
	local hubDir = args.companyDir+"/OpcHub", //companyDir is the one spelling of the company root - see paths-common.
	local binDir = "$(ExeDir)",
	local appServerDll = binDir+"/Jde.DB.Sqlite.AppServer.dll",
	sqlType: "sqlite",
	logsDir: hubDir,
	dbServers: {
		dataPaths: [hubDir+"/sql"],
		scriptPaths: [hubDir+"/sql"],
		localhost:{
			driver: binDir+"/Jde.DB.Sqlite.dll",
			connectionString: null,
			username: null,
			password: null,
			schema: null,
			catalogs: {
				master: { // n/a for sqlite - the only real field is the db path.
					path: hubDir+"/OpcHub.db",
					schemas:{
						dbo:{ // n/a for sqlite
							access:{ meta: hubDir+"/access-meta.jsonnet", ql: hubDir+"/access-ql.jsonnet", prefix: "access_", dynamicLib: appServerDll },
							app:{ meta: hubDir+"/app-meta.jsonnet", prefix: "app_", dynamicLib: appServerDll },
							gateway:{ meta: hubDir+"/opcGateway-meta.jsonnet", prefix: "gateway_", dynamicLib: binDir+"/Jde.DB.Sqlite.OpcGateway.dll" }
						}
					}
				}
			}
		}
	}
}
