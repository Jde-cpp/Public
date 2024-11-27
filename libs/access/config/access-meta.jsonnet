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

local smallSequenced = types.uint16+{ sequence: true, sk:0, i:0 };
local pkSequenced = types.uint+{ sequence: true, sk:0, i:0 };
local valuesColumns = { name: types.varchar+{ length: 256, i:10 } };

local valuesNK = ["name"];

local targetColumns = valuesColumns+{
	target:types.varchar+{ length: valuesColumns.name.length, i:20 },
	attributes: types.uint16+{ nullable: true, i:30 },
	created: types.dateTime+{ insertable: false, updateable: false, default: sqlFunctions.now.name, i:40 },
	updated: types.dateTime+{ nullable: true, insertable: false, updateable: false, i:50 },
	deleted:types.dateTime+{ nullable: true, insertable: false, updateable: false, i:60 },
	description: types.varchar+{ length: 2048, nullable: true, i:70 }
};

local targetNKs = [valuesNK, ["target"]];
local rights = ["None", "Create", "Read", "Update", "Delete", "Purge", "Administer", "Subscribe", "Execute"];
local defaultOps = ["Create", "Read", "Update", "Delete", "Purge", "Administer"];
{
	local tables = self.tables,
	views:{
		groupMembers:{
			comment: "Group Members",
			columns:{
				groupId: tables.identityGroups.columns.identityId+{ criteria: null },
				memberId: tables.identityGroups.columns.memberId,
				providerId: tables.providers.columns.providerId+{ pkTable: "providers", nullable:true },
				isGroup: tables.identities.columns.isGroup
			}+targetColumns,
			naturalKeys: tables.identities.naturalKeys,
		},
		providersQL:{
			columns: {
				providerId: tables.providers.columns.providerId,
				name: tables.providers.columns.target+{ comment: "if provider_id=OpcServer: the specific opcServer target, otherwise providers[provider_id].name " },
			},
			naturalKeys: [["provider_id", "name"]]
		}
	},
	tables:{
		identities:{
			comment: "Group or User",
			columns:{
				identityId: pkSequenced,
				providerId: tables.providers.columns.providerId+{ pkTable: "providers", nullable:true, i: 15 },
				isGroup: types.bit+{ default: false, i: 101 },
			}+targetColumns,
			naturalKeys:[ ["name","provider_id"], ["target"] ],
			data: [
				{ id:1, attributes:5, name:"Everyone", target:"everyone", isGroup:true },
				{ id:2, attributes:6, name:"Users", target:"users", isGroup:true }
			],
			ops: ["None"]
		},
		users:{
			columns: {
				identityId: tables.identities.columns.identityId+{ pkTable: {name: "identities"} },
				loginName: valuesColumns.name+{ nullable: true },
				password: types.varbinary+{ length: 2048, encrypted:true, nullable: true, i:101 },
				modulus: types.varchar+{ length: 2048, nullable:true, comment: "Used for RSA", i:102 },
				exponent: types.uint+{ nullable:true, comment: "Used for RSA", i:103 }
			},
			ops: ["Create", "Read", "Update", "Delete", "Purge", "Administer", "Execute"],
			extends: "identities"
		},
		identityGroups:{
			columns: {
				identityId: tables.identities.columns.identityId+{ pkTable: "identities", criteria: "is_group"  },
				memberId: 	tables.identities.columns.identityId+{ pkTable: "identities", name: "member_id", sk: 1, i:1 },
			},
			map: {parentId:"identity_id", childId:"member_id"},
			extends: "identities"
		},
		providerTypes:{
			columns: {
				providerTypeId: types.uint8+{sk:0, i:0},
			}+valuesColumns,
			data: [
				{id:1, name:"Google"},
				{id:2, name:"Facebook"},
				{id:3, name:"Amazon"},
				{id:4, name:"Microsoft"},
				{id:5, name:"VK"},
				{id:6, name:"Key", comment:"RSA connection"},
				{id:7, name:"OpcServer"}
			],
		},
		providers:{
			columns: {
				providerId: smallSequenced,
				providerTypeId: tables.providerTypes.columns.providerTypeId+{ pkTable: "provider_types", i:1 },
				target: targetColumns.target+{ nullable: true, comment: "Points to target in another table (eg OpcServer)" }
			},
			naturalKeys:[["provider_type_id","target"]],
			purgeProc: "provider_purge",
			data: [{id:1, providerTypeId:1}, {id:2, providerTypeId:2}, {id:3, providerTypeId:3}, {id:4, providerTypeId:4}, {id:5, providerTypeId:5}, {id:6, providerTypeId:6} ],
			qlView: "providers_ql",
			ops: ["None"]
		},
		resources:{
			columns: {
				resourceId: smallSequenced,
				schemaName: valuesColumns.name+{ i:1 },
				criteria: types.varchar+{ nullable: true, length:1024, i:100 },
				allowed: tables.rights.columns.rightId+{ pkTable: "rights", i:101, nullable:true, comment: "available rights for this resource" },
				denied: tables.rights.columns.rightId+{ pkTable: "rights", i:102, nullable:true, comment: "available rights for this resource" },
			}+targetColumns,
			ops: ["None"]//handled through admin op.
		},
		permissions:{
			columns: {
				permissionId: pkSequenced,
				isRole: types.bit+{ default: false, i: 101 },
			},
			ops: ["None"]
		},
		permissionRights:{
			columns: {
				permissionId: tables.permissions.columns.permissionId+{ pkTable: "permissions", i:0, sk:0 },
				resourceId: tables.resources.columns.resourceId+{ pkTable: "resources", i:1 },
				allowed: tables.rights.columns.rightId+{ pkTable: "rights", i:2 },
				denied: tables.rights.columns.rightId+{ pkTable: "rights", i:3 },
			},
			ops: ["None"]
		},
		roles:{
			columns: {
				roleId: tables.permissions.columns.permissionId+{ insertable:false, pkTable: "permissions", i:0, sk:0 },
			}+targetColumns,
			customInsertProc: true,
			purgeProc: "role_purge",
			addProc: "role_add",
			removeProc: "role_remove",
			naturalKeys: targetNKs
		},
		roleMembers:{
			columns: {
				roleId: tables.permissions.columns.permissionId+{ pkTable: "roles", sk: 0, i:0 },
				memberId: tables.permissions.columns.permissionId+{ pkTable: "permissions", sk:1, i:1 },
			},
			extends: "roles",
			map: {parentId:"role_id", childId:"member_id"},
			ops: ["None"]
		},
		rights:{
			columns: {
				rightId: types.uint8+{ sk:0, i:0 },
				name: types.varchar+{ length: 11, i:1 }
			},
			flagsData: rights,
			ops: ["None"]
		},
		acl:{
			columns: {
				identityId: tables.identities.columns.identityId+{ pkTable: {name:"identities"}, sk: 0, i:0 },
				permissionId: tables.permissions.columns.permissionId+{ pkTable: "permissions", sk: 1, i:1 }
			},
			customInsertProc: true,
			map:: { parentId:"identity_id", childId:"permission_id" },//not a map, but connector.
			ops: ["None"] //handled through admin op.
		}
	}
}