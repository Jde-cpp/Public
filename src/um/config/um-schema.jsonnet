local types = {
	binary: {type: "Binary"},
	bit: {type:"Bit", length: 1},
	blob: {type: "Blob"},
	char: {type: "Char"},
	cursor: {type: "Cursor"},
	dateTime: {type: "DateTime", length:: 64},
	decimal: {type: "Decimal"},
	float: {type:"Float", length:: 64},
	guid: {type: "Guid", length:: 128},
	image: {type: "Image"},
	int: {type:"Int", length:: 32},
	int8: {type: "Int8", length:: 8},
	int16: {type:"Int16", length:: 16},
	long: {type: "Long", length:: 64},
	money: {type: "Money", length:: 64},
	ntext: {type: "NText"},
	numeric: {type: "Numeric"},
	refCursor: {type: "RefCursor", length:: 64},
	smallDateTime: {type: "SmallDateTime", length:: 32},
	smallFloat: {type:"SmallFloat", length:: 32},
	tchar: {type: "TChar"},
	text: {type: "Text"},
	timeSpan: {type: "TimeSpan", length:: 64},
	uint: {type:"UInt", length:: 32},
	uint8: {type: "UInt8", length:: 8},
	uint16: {type: "UInt16", length:: 16},
	ulong: {type: "ULong", length:: 64},
	uri: {type: "Uri"},
	varbinary: {type: "VarBinary"},
	varchar: {type: "Varchar"},
	varTChar: {type: "VarTChar"},
	varWChar: {type: "VarWChar"},
	wchar: {type: "WChar"},
};
local sqlFunctions = {
	now: { name: "$now" }
};
local columns = {
	created: types.dateTime+{ name: "created", insertable: false, updateable: false, default: sqlFunctions.now.name },
	description: types.varchar+{ name: "description", length: 2048, nullable: true },
	id: types.uint,
	name:types.varchar+{ name: "name", length: 256 },
	password: types.varbinary+{ name:"password", length: 2048, encrypted:true, nullable: true },
	smallId: types.uint16,
	target:types.varchar+{ name: "target", length: 255 },
};
local valuesTable = {
	columns: [
		columns.id,
		columns.name
	],
	surrogateKeys:["$id"],
	naturalKeys:[["name"]]
};
local	smallValues={
	columns: [
		columns.smallId+{ sequence: true },
		columns.name+{ name: "name" }
	],
	surrogateKeys:["id"],
	naturalKeys:[["name"]]
};

local dataTable = valuesTable+{
	columns: [
		valuesTable.columns[0]+{ sequence: true },
		valuesTable.columns[1]
	],
};
local defaultTable = dataTable+{
	columns+: [
		types.uint16+{ name: "attributes", nullable: true },
		columns.created,
		types.dateTime+{ name: "updated", nullable: true, insertable: false, updateable: false },
		types.dateTime+{ name: "deleted", nullable: true, insertable: false, updateable: false }
	]
};
local targetTable = defaultTable+{
	columns: [
			defaultTable.columns[0],
			defaultTable.columns[1],
			columns.target,
			defaultTable.columns[2],
			defaultTable.columns[3],
			defaultTable.columns[4],
			defaultTable.columns[5],
			columns.description
	],
	naturalKeys:[ ["target"] ]
};
{
	um:{
		scripts: ["providers_ql.sql", "provider_purge.sql", "user_insert.sql", "user_insert_login.sql", "user_insert_key.sql"],
		views:{
			providersQL:{
				columns: [
					types.uint+{ name: "provider_id", sequence: true },
					columns.name+{ nullable: true },
				]
				naturalKeys: [["provider_id", "name"]]
			}
		},
		tables:{
			local tables2 = self,
			entities:targetTable+{
				notes: "Group or User",
				columns: [
					targetTable.columns[0]+{ name: "entity_id" },
					targetTable.columns[1],
					tables2.providers.columns[0]+{ parent: "providers" },
					targetTable.columns[2],
					targetTable.columns[3],
					targetTable.columns[4],
					targetTable.columns[5],
					targetTable.columns[6],
					targetTable.columns[7],
					types.bit+{ name: "is_group", default: false },
				],
				naturalKeys:[ ["name","provider_id"], ["target"]],
				surrogateKeys:["entity_id"],
				data: [
					{ id:1, attributes:5, name:"Everyone", target:"everyone", isGroup:true },
					{ id:2, attributes:6, name:"Users", target:"users", isGroup:true }
				]
			},
			users:{
				columns: [
					tables2.entities.columns[0]+{ fk: "entities" },
					columns.name+{ name: "login_name", nullable: true },
					columns.password,
					types.varchar+{ name: "modulus", nullable:true, length: 2048, comment: "Used for RSA" },
					types.uint+{ name: "exponent", nullable:true, comment: "Used for RSA" }
				],
				surrogateKeys:["entity_id"]
			},
			groups:{
				columns: [
					tables2.entities.columns[0]+{ fk: "entities", criteria: "is_group" },
					tables2.entities.columns[0]+{ fk: "entities", name: "member_id" },
				],
				map: {parentId:"entity_id", childId:"member_id"},
				surrogateKeys:["entity_id", "member_id"]
			},
			providerTypes:valuesTable+{
				columns: [
					valuesTable.columns[0]+{ name: "provider_type_id" },
					valuesTable.columns[1]
				],
				data: [
					{id:1, name:"Google"},
					{id:2, name:"Facebook"},
					{id:3, name:"Amazon"},
					{id:4, name:"Microsoft"},
					{id:5, name:"VK"},
					{id:6, name:"Key"},
					{id:7, name:"Certificate"},
					{id:8, name:"OpcServer"}
				],
				surrogateKeys:["provider_type_id"]
			},
			providers:{
				columns: [
					types.uint+{ name: "provider_id", sequence: true },
					tables2.providerTypes.columns[0]+{ parent: "providerTypes" },
					columns.target+{ nullable: true, notes: "Points to target in another table (eg OpcServer)" }
				],
				surrogateKeys:["provider_id"],
				naturalKeys:[["provider_type_id","target"]],
				purgeProc: "um_provider_purge",
				data: [{id:1, providerTypeId:1}, {id:2, providerTypeId:2}, {id:3, providerTypeId:3}, {id:4, providerTypeId:4}, {id:5, providerTypeId:5}, {id:6, providerTypeId:6}, {id:7, providerTypeId:7} ],
				qlView: "umProvidersQL"
			},

		/*	"umRoles":{
				"parent": "$description",
				data: [
					{id:1, "attributes":4, name:"User Management", "target": "user_management" }
				]
			},
			"umEntityRoles":{
				columns: [
					{ name: "entityId", type: "entities" },
					{ name: "roleId", type: "umRoles" }
				],
				surrogateKeys:["entityId","roleId"],
				data: [{"entityId":1, "roleId":1 }]
			},
			"umPermissions":{
				columns: [
					{ name: "id", sequence: true },
					{ name: "apiId", type: "umApis" },
					{ name: "name", type: "name?", "qlAppend":"apiId", comment: "null=default, name:particular permission, api=um, name:'change password'" }
				],
				surrogateKeys:["id"],
				naturalKeys:["apiId","name"],
				data: [{id:1, "apiId":1}, {id:2, "apiId":3} ]
			},
			"umRights":{
				"parent": "$values",
				"flagsData": ["None","Administer", "Write", "Read"]
			},
			"umRolePermissions":{
				columns: [
					{ name: "roleId", type: "umRoles", "updateable":false },
					{ name: "permissionId", type: "umPermissions", "updateable":false },
					{ name: "rightId", type: "umRights", "default":"4" }
				],
				surrogateKeys:["roleId","permissionId"],
				data: [{"roleId":1, "permissionId":1, "rightId": 7}]
			}*/
		}
	}
}