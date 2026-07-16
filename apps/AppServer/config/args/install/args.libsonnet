{
	logsDir: "$(ProgramData)/jde-cpp/AppServer",
	dbServers: {
		dataPaths: ["$(ProgramData)/jde-cpp/AppServer/sql"],
		scriptPaths: ["$(ProgramData)/jde-cpp/AppServer/sql"],
		localhost:{
			driver: "$(ProgramW6432)/jde-cpp/AppServer/Jde.DB.Odbc.dll",
			connectionString: "DSN=jde",
			username: null,
			password: null,
			schema: null,
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
						app:{
							app:{
								meta: "$(ProgramData)/jde-cpp/AppServer/app-meta.jsonnet",
								prefix: ""
							},
						}
					}
				}
			}
		}
	}
}