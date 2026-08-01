local paths = import '../../../../../libs/db/config/paths-common.libsonnet';
paths + {
	local args = self,
	local appDir = args.companyDir+"/AppServer", //companyDir is the one spelling of the company root - see paths-common.
	logsDir: appDir,
	dbServers: {
		dataPaths: [appDir+"/sql"],
		scriptPaths: [appDir+"/sql"],
		localhost:{
			driver: "$(ProgramW6432)/Jde-Cpp/AppServer/Jde.DB.Odbc.dll", //program files, not programData - a different root, so not companyDir.
			connectionString: "DSN=jde",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				jde: {
					schemas:{
						acc:{
							access:{
								meta: appDir+"/access-meta.jsonnet",
								ql: appDir+"/access-ql.jsonnet",
								prefix: ""
							}
						},
						app:{
							app:{
								meta: appDir+"/app-meta.jsonnet",
								prefix: ""
							},
						}
					}
				}
			}
		}
	}
}
