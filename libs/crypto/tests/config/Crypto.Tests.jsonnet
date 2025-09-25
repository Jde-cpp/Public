{
	testing:{
		tests:: "OpenSslTests.Main"
	},
	cryptoTests:{
		clear: false
	},
	logging:{
		spd:{
			tags: {
				default:"information",
				trace:["test", "exception"],
				debug:["settings"],
				information:[],
				warning:[],
				"error":[],
				critical:[]
			},
			sinks:{
				console:{},
				file:{ path: "$(JDE_TEST_LOG_DIR)", md: false }
			}
		}
	}
}