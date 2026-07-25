//Dialect-only coverage for the odbc driver: the SQL Server dialect is the base DB::Syntax, so nothing here opens a
//DSN.  No dbServers block, so addJdeTest's `-include=args/sqlite -arg path=:memory:` is inert (no args/ dir).
local logsDir = std.extVar("logsDir");
{
	testing:{
		tests:: "SyntaxTests.*"
	},
	instanceName: "OdbcTests",
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
