{
	sqlType: "sqlServer",
	logsDir: "$(ProgramData)/jde-cpp/OpcGateway",
	dbServers: {
		scriptPaths: ["$(ProgramData)/jde-cpp/OpcGateway/sql"],
		localhost:{
			driver: "$(ProgramW6432)/jde-cpp/AppServer/Jde.DB.Odbc.dll",
			connectionString: "DSN=jde",
			username: null,
			password: null,
			schema: null,
			catalogs: {
				jde: {
					schemas:{
						_access:{
							access:{
								meta: "$(ProgramData)/jde-cpp/AppServer/access-meta.jsonnet"
							}
						},
						opc:{
							opc:{
								meta: "$(ProgramData)/jde-cpp/OpcGateway/opcGateway-meta.jsonnet",
								prefix: ""
							}
						}
					}
				}
			}
		}
	}
}