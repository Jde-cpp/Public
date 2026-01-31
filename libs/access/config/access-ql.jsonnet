local String = { kind:"SCALAR", name:'String' };
local NonNullString = { kind: 'NON_NULL', name:null, ofType:String };
local Id = { kind: 'NON_NULL', name:null, ofType:{kind:"SCALER", name:"ID"} };
local Attributes = { kind:"SCALAR", name: 'UInt' };
local DateTime = { kind:"SCALAR",name:'DateTime' };
local NonNullDateTime = { kind:'NON_NULL',name:null, ofType: DateTime };
{
	Grouping:{
		fields: [
				{ name: 'id', type: Id },
				{ name: 'name', type: NonNullString },
				{ name: 'attributes',type: Attributes },
				{ name: 'created', type: NonNullDateTime },
				{ name: 'updated', type: DateTime },
				{ name: 'deleted', type:DateTime },
				{ name: 'target',  type:String },
				{ name: 'description', type: String },
				{ name: 'provider', type: {kind:"ENUM", name:'Provider'} },
				{ name: 'members', type: {name: null, kind: "LIST", ofType: {name: "Identity", kind: "UNION"}} }
		]},
	User:{
		fields: [
				{ name: 'id', type: Id },
				{ name: 'name', type: NonNullString },
				{ name: 'provider', type: {kind:"ENUM", name:'Provider'} },
				{ name: 'target',  type: NonNullString },
				{ name: 'attributes',type: Attributes },
				{ name: 'created', type: NonNullDateTime },
				{ name: 'updated', type: DateTime },
				{ name: 'deleted', type:DateTime },
				{ name: 'description', type: String },
				{ name: 'isGroup', type: {kind:'NON_NULL', name:null, ofType:{kind:'SCALAR', name:'Boolean'}} },
				{ name: 'loginName', type: String },
				{ name: 'modulus', type: String },
				{ name: 'exponent', type: Attributes }
		]}
}