local args = import 'args.libsonnet';
local logsDir = args.logsDir;
{
	testing:{
		tests:: "RoleTests.Crud",
		recreateDB: true
	},
	dbServers:{
		scriptPaths: [args.repoSourceDir+"/libs/access/config/sql/"+args.sqlType],
		dataPaths: [args.repoSourceDir+"/libs/access/config"],
		sync:: true,
		localhost:{
			driver: args.dbServers.localhost.driver,
			connectionString: args.dbServers.localhost.connectionString,
			username: args.dbServers.localhost.username,
			password: args.dbServers.localhost.password,
			schema: args.dbServers.localhost.schema,
			catalogs: args.dbServers.localhost.catalogs
		}
	},
	logging:{
		spd:{
			tags: {
				default: "Information",
				test: "Trace",
				access: "Trace",
				app: "Trace",
				settings: "Debug",
				ql: "Debug",
				sql: "Debug",
				exception: "Trace"
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