{
	testing:{
		tests: "*"
	},
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
				console:{}
			}
		}
	},
	workers:{
		executor: {threads: 2}
	}
}
