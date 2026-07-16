local logsDir = std.extVar("logsDir");
{
	testing:{
		tests:: "FileTests.WriteExactChunkMultiple",
		file: logsDir + "/tests/test.txt"
	},
	cryptoTests:{
		clear: false
	},
	logging:{
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
				file:{ path: logsDir, md: false, pattern: "%^%3!l%$-%H:%M:%S.%e \\033]8;;file://%g#%#\\a%v\\033]8;;\\a" }
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