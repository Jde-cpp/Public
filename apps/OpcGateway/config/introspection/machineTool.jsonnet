{
	"ns=9;i=65":{
		description: "MachineTool MachineToolType Mandatory Nodes",
		enumValues: [
			{ id:0, name:"Manual", description:"The machine tool is controlled manually, by the operator. Depending on technology specific norms, the maximum axis movement speeds of the machine tool are limited."},
			{ id:1, name:"Automatic", description:"Operating mode for the automatic, programmed and continuous operation of the machine. Manual loading and unloading workpieces are possible when the automatic program is stopped. Axis movement speeds are fully available to the machine tool’s ability."},
			{ id:2, name:"Setup", description:"Depending on technology specific norms, the maximum axis movement speeds of the machine tool are limited. In this mode, the operator can make settings for the subsequent work processes."},
			{ id:3, name:"AutoWithManualIntervention", description:"Operating mode with the possibility of manual interventions in the machining process as well as limited automatic operation started by the operator. Depending on technology specific norms, the maximum axis movement speeds of the machine tool are limited."},
			{ id:4, name:"Service", description:"Operating mode for service purposes. This mode shall not be used for manufacturing any parts. This mode shall only be used by authorized personnel.<"},
			{ id:5, name:"Other", description:"The machine operation mode is different from the values defined in this enumeration."}
		]
	},
}
