local logsDir = std.extVar("logsDir");
{
	testing:{
		tests:: "TrustStoreTests.*",
		file: logsDir + "/tests/test.txt"
	},
	cryptoTests:{
		clear: false
	},
	logging:{
		breakLevel: "Critical",
		spd:{
			tags: {
				default: "Information",
				app: "Trace",
				exception: "Trace",
				io: "Information",
				test: "Trace",
				settings: "Trace"
			},
			sinks:{
				console:{},
				file:{ path: logsDir, md: false }
			}
		},
		memory:{
			tags: {
				default: "Debug"
			}
		}
	},
	workers:{
		executor: {threads: 2},
		io: {chunkByteSize: 10, threads: 2}
	}
}