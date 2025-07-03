{
	scriptPaths: ["$(ProgramData)/jde-cpp/OpcGateway/sql"],
	sqlType: "sqlServer",
	logDir: "$(ProgramData)/jde-cpp/OpcGateway",
	dbDriver: "$(ProgramW6432)/jde-cpp/AppServer/Jde.DB.Odbc.dll",
	dbConnectionString: "DSN=jde",
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