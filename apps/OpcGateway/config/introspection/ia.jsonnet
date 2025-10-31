{
	"ns=3;i=3002":{
		description: "Contains the values used to indicate how a stacklight (as a whole unit) is used.",
		enumValues: [
			{ id:0, name:"Segmented", description:"Stacklight is used as stack of individual lights"},
			{ id:1, name:"Levelmeter", description:"Stacklight is used as level meter"},
			{ id:2, name:"Running_Light", description:"The whole stack acts as a running light"},
			{ id:3, name:"Other", description:"Stacklight is used in a way not defined in this version of the specification."}
		]
	},
	"ns=3;i=3004":{
		description: "Holds the possible color values for stacklight lamps.",
		enumValues: [
			{ id:0, name:"Off", description:"Element is disabled."},
			{ id:1, name:"Red", description:"Red color."},
			{ id:2, name:"Green", description:"Green color."},
			{ id:3, name:"Blue", description:"Blue color."},
			{ id:4, name:"Yellow", description:"Yellow color."},
			{ id:5, name:"Purple", description:"Purple color."},
			{ id:6, name:"Cyan", description:"Cyan color."},
			{ id:7, name:"White", description:"White color."}
		]
	},
	"ns=3;i=3005":{
		description: "Contains the values used to indicate in what way a lamp behaves when switched on.",
		enumValues: [
			{ id:0, name:"Continuous", description:"This value indicates a continuous light."},
			{ id:1, name:"Blinking", description:"This value indicates a blinking light (blinking in regular intervals with equally long on and off times)."},
			{ id:2, name:"Flashing", description:"This value indicates a flashing light (blinking in intervals with longer off times than on times, per interval multiple on times are possible)."},
			{ id:3, name:"Other", description:"The light is handled in a way not defined in this version of the specification."}
		]
	}
}