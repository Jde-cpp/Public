local String = { kind:"SCALAR", name:'String' };
local NonNullString = { kind: 'NON_NULL', name:null, ofType:String };
local Attributes = { kind:"SCALAR", name: 'PositiveInt' };
local NonNullDateTime = { kind:"SCALAR",name:'DateTime' };
local DateTime = { kind:'NON_NULL',name:null, ofType: NonNullDateTime };
{
Group:{
	fields: [
			{ name: 'id', type: NonNullString }, 
			{ name: 'name', type: NonNullString }, 
			{ name: 'attributes',type: Attributes }, 
			{ name: 'created', type: NonNullDateTime }, 
			{ name: 'updated', type: DateTime },
			{ name: 'deleted','type':DateTime }, 
			{ name: 'target', 'type':NonNullString }, 
			{ name: 'description', type: String}, 
			{ name: 'provider', type: {kind:"ENUM", name:'Provider'}}, 
			{ name: 'members', type: {name: null, kind: "LIST", ofType:{ name: null, kind: "NON_NULL", ofType: { name: "Entity", kind: "UNION" } } } }
	]}
}