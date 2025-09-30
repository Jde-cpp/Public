{
	testing:{
		tests: "LogGeneralTests.ArgsNotCalled",
		file: "$(JDE_BUILD_DIR)/tests/test.txt"
	},
	cryptoTests:{
		clear: false
	},
	logging:{
		spd:{
			tags: {
				trace:["test", "io", "exception", "app"],
				debug:["settings"],
				information:[],
				warning:[],
				"error":[],
				critical:[]
			},
			sinks:{
				console:{},
				file:{ path: "$(JDE_BUILD_DIR)", md: false, pattern: "%^%3!l%$-%H:%M:%S.%e %v" }
			}
		},
		memory:{ default: "trace" }
	},
	workers:{
		executor: {threads: 2},
		io: {chunkByteSize: 10, threads: 2}
	}
}