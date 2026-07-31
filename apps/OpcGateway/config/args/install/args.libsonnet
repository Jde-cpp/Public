local paths = import '../../../../../libs/db/config/paths-common.libsonnet';
paths + {
	local args = self,
	local gatewayDir = args.companyDir+"/OpcGateway", //companyDir is the one spelling of the company root - see paths-common.
	local appServerDir = args.companyDir+"/AppServer",
	sqlType: "sqlServer",
	logsDir: gatewayDir,
	dbServers: {
		scriptPaths: [gatewayDir+"/sql"],
		localhost:{
			driver: "$(ProgramW6432)/Jde-Cpp/AppServer/Jde.DB.Odbc.dll", //program files, not programData - a different root, so not companyDir.
			connectionString: "DSN=jde",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				jde: {
					schemas:{
						_access:{
							access:{
								meta: appServerDir+"/access-meta.jsonnet"
							}
						},
						opc:{
							opc:{
								meta: gatewayDir+"/opcGateway-meta.jsonnet",
								prefix: ""
							}
						}
					}
				}
			}
		}
	}
}
