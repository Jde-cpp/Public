//Data-source-free Jde.App.Shared coverage - the archive filter/sort/json, the string cache and the proto builders.
//No dbServers block: nothing here opens a data source, so addJdeTest's `-include=args/sqlite -arg path=:memory:` is
//inert (there is no args/ dir to import).
local logsDir = std.extVar("logsDir");
{
	testing:{
		tests:: "*"
	},
	instanceName: "AppTests",
	logging:{
		spd:{
			tags: {
				default: "Information",
				app: "Trace",
				exception: "Trace",
				test: "Trace",
				settings: "Debug",
				externalLogger: "Debug"
			},
			sinks:{
				console:{},
				file:{ path: logsDir, md: false }
			}
		},
		//The `binary` logger of the logSetting query/mutation.  delay/size are never reached in a sub-second run, so it
		//buffers and Shutdown flushes once - no timer round, no archive round.
		proto:{
			path: logsDir + "/app-tests",
			timeZone: "America/New_York", //explicit: LogTests asserts the <root>/2025/1/<day> archive folders, which are local dates.
			delay: "PT1M",
			tags: {
				default: "Information",
				externalLogger: "None"
			}
		}
	},
	http:{
		port: 5010 //FromClient::Instance stamps this into the instance message; never bound - nothing here listens.
	},
	workers:{
		executor: {threads: 2}
	}
}
