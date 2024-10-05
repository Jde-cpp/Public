local String = { kind:"SCALAR", name:'String' };
local NonNullString = { kind: 'NON_NULL', name:null, ofType:String };
local Id = { kind: 'NON_NULL', name:null, ofType:{kind:"SCALER", name:"ID"} };
local Attributes = { kind:"SCALAR", name: 'PositiveInt' };
local DateTime = { kind:"SCALAR",name:'DateTime' };
local NonNullDateTime = { kind:'NON_NULL',name:null, ofType: DateTime };
{
Group:{
	fields: [
			{ name: 'id', type: Id }, 
			{ name: 'name', type: NonNullString }, 
			{ name: 'attributes',type: Attributes }, 
			{ name: 'created', type: NonNullDateTime }, 
			{ name: 'updated', type: DateTime },
			{ name: 'deleted', type:DateTime }, 
			{ name: 'target',  type:Id }, 
			{ name: 'description', type: String }, 
			{ name: 'provider', type: {kind:"ENUM", name:'Provider'} }, 
			{ name: 'members', type: {name: null, kind: "LIST", ofType:{name: null, kind: "NON_NULL", ofType: {name: "Entity", kind: "UNION"}}} }
	]}
}