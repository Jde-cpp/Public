{
	testing:{
		tests:: "OpenSslTests.Main"
	},
	cryptoTests:{
		clear: false
	},
	logging:{
		tags: {
			trace:["test"],
			debug:["settings"],
			information:[],
			warning:[],
			"error":[],
			critical:[]
		},
		sinks:{
			console:{},
			file:{ path: "/tmp", md: false }
		},
	},
	workers:{
		drive: {threads: 1}
	}
}