{
	testing:{
		tests:: "ThreadingTest.*"
	},
	logging:{
		spd:{
			tags:{
				default: "Information",
				app: "Trace",
				test: "Trace"
			},
			sinks:{
				console:{}
			}
		},
		memory:{
			tags:{
				default: "Debug"
			}
		}
	},
	workers:{
		executor: {threads: 2},
		io: {chunkByteSize: 10, threads: 2}
	}
}
