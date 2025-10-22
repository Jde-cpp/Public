{
	"ns=10;i=3000":{
		description: "This enumeration indicates the function of a specific feedstock.",
		enumValues: [
			{ id:0, name:"Undefined", description:"The function of the feedstock is unknown."},
			{ id:1, name:"Main", description:"The feedstock is used for production and is part of the finished part."},
			{ id:2, name:"Ancillary", description:"The feedstock is used for production but removed before the part is finished."},
			{ id:3, name:"Consumable", description:"The feedstock is consumed during the production e.g., process gas."}
		]
	},
}
