local paths = import '../../../../../libs/db/config/paths-common.libsonnet';
//The installed layout: one product dir holding the AppServer's and the gateway's meta/sql (the installer that populates it
//is apps/OpcGateway/setup, still on the split layout).  paths-common only - the service starts without -tests, so no ext vars.
paths + {
	local args = self,
	local hubDir = args.companyDir+"/OpcHub",
	sqlType: "sqlServer",
	logsDir: hubDir,
	dbServers: {
		dataPaths: [hubDir+"/sql"],
		scriptPaths: [hubDir+"/sql"],
		localhost:{
			driver: "$(ProgramW6432)/Jde-Cpp/OpcHub/Jde.DB.Odbc.dll", //program files, not programData - a different root, so not companyDir.
			connectionString: "DSN=jde",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				jde: {
					schemas:{
						acc:{
							access:{ meta: hubDir+"/access-meta.jsonnet", ql: hubDir+"/access-ql.jsonnet", prefix: "" }
						},
						app:{
							app:{ meta: hubDir+"/app-meta.jsonnet", prefix: "" }
						},
						opc:{
							gateway:{ meta: hubDir+"/opcGateway-meta.jsonnet", prefix: "" }
						}
					}
				}
			}
		}
	}
}
