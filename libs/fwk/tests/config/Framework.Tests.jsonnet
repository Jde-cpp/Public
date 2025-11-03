{
	testing:{
		tests:: "LogGeneralTests.ArgsNotCalled",
		file: "$(JDE_BUILD_DIR)/tests/test.txt"
	},
	cryptoTests:{
		clear: false
	},
	logging:{
		spd:{
			tags: {
				trace:["exception", "app", "io"],
				debug:["test", "settings"],
				information:[],
				warning:[],
				"error":[],
				critical:[]
			},
			sinks:{
				console:{},
				file:{ path: "$(JDE_BUILD_DIR)", md: false, pattern: "%^%3!l%$-%H:%M:%S.%e \\033]8;;file://%g#%#\\a%v\\033]8;;\\a" }
			}
		},
		memory:{ default: "trace" }
	},
	workers:{
		executor: {threads: 2},
		io: {chunkByteSize: 10, threads: 2}
	}
}