{
	dataPaths: ["$(ProgramData)/jde-cpp/AppServer/sql"],
	scriptPaths: ["$(ProgramData)/jde-cpp/AppServer/sql"],
	logDir: "$(ProgramData)/jde-cpp/AppServer",
	dbDriver: "$(ProgramW6432)/jde-cpp/AppServer/Jde.DB.Odbc.dll",
	dbConnectionString: "DSN=jde",
	catalogs: {
		jde: {
			schemas:{
				acc:{
					access:{
						meta: "$(ProgramData)/jde-cpp/AppServer/access-meta.jsonnet",
						ql: "$(ProgramData)/jde-cpp/AppServer/access-ql.jsonnet",
						prefix: ""
					}
				},
				log:{
					log:{
						meta: "$(ProgramData)/jde-cpp/AppServer/log-meta.jsonnet",
						prefix: ""
					},
				}
			}
		}
	}
}