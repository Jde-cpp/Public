//Data-source-free Jde.QL coverage - the parser, Input arg/variable handling and the in-memory filters.  No dbServers
//block: nothing here opens a data source, so addJdeTest's `-include=args/sqlite -arg path=:memory:` is inert (there
//is no args/ dir to import).
local logsDir = std.extVar("logsDir");
{
	testing:{
		tests:: "*"
	},
	instanceName: "QLTests",
	logging:{
		spd:{
			tags: {
				default: "Information",
				app: "Trace",
				exception: "Trace",
				test: "Trace",
				settings: "Debug",
				ql: "Debug"
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
