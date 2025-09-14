local common = import 'common-meta.libsonnet';
{
	local types = common.types,
	local tables = self.tables,
	local views = self.views,
	local nodeId = types.ulong,
	local nodeIdNull = nodeId+{nullable: true},
	local nsIndex = types.uint16,
	local nodeTableProps = {
		extends: "node_ids",
		customInsertProc: true,
		naturalKeys: [ ["parent_node_id", "browse_id"] ]
	},
	local nodeColumns = {
		nodeId: nodeId+{ sk:0, pkTable:"node_ids", i:200, insertable: false },
		parentNodeId: nodeIdNull+{ pkTable:"node_ids", i:201 },
		refTypeId: nodeId+{ pkTable:"node_ids", i:202 },
		browseId: tables.browseNames.columns.browseId+{ sk:null, pkTable: "browse_names", i:203 },
		typeDefId: nodeId+{ pkTable:"node_ids", i:204 },
		specified: types.uint+{ i:205, nullable: true },
		name: common.targetColumns.name+{ i:206 },
		description: common.targetColumns.description+{ i:207, nullable: true },
		writeMask: types.uint+{ i:208, nullable: true },
		userWriteMask: types.uint+{ i:209, nullable: true },
		created: common.targetColumns.created+{ i:210 },
		deleted: common.targetColumns.deleted+{ i:211 },
		updated: common.targetColumns.updated+{ i:212 }
	},
	views:{
		objectNodes:{
			comment: "Objects nodes of a server",
			columns:{serverId: tables.servers.columns.serverId}
				+tables.nodeIds.columns
				+common.filter(tables.objects.columns, "nodeId"),
			customInsertProc: true
		},
		objectTypeNodes:{
			comment: "Objects nodes of a server",
			columns:{serverId: tables.servers.columns.serverId}
				+tables.nodeIds.columns
				+common.filter(tables.objectTypes.columns, "nodeId"),
			customInsertProc: true
		},
		serverBrowseNames:{
			comment: "Browse names of a server",
			columns:{
				serverId: tables.servers.columns.serverId,
				browseId: tables.browseNames.columns.browseId+{ i:1 },
				ns: tables.browseNames.columns.ns+{ i:2 },
				name: tables.browseNames.columns.name+{ i:3 }
			}
		},
		serverNodeIds:{
			comment: "Nodes of a server",
			columns:{
				serverId: tables.servers.columns.serverId+{ i:0 },
				nodeId: tables.nodeIds.columns.nodeId+{ i:1 },
				ns: tables.nodeIds.columns.ns+{ i:2 },
				number: tables.nodeIds.columns.number+{ i:3 },
				string: tables.nodeIds.columns.string+{ i:4 },
				guid: tables.nodeIds.columns.guid+{ i:5 },
				bytes: tables.nodeIds.columns.bytes+{ i:6 },
				namespaceUri: tables.nodeIds.columns.namespaceUri+{ i:7 },
				serverIndex: tables.nodeIds.columns.serverIndex+{ i:8 },
				isGlobal: tables.nodeIds.columns.isGlobal+{ i:9 }
			},
			customInsertProc: true
		},
		variableNodes:{
			comment: "Variable nodes of a server",
			columns:{serverId: tables.servers.columns.serverId}
				+tables.nodeIds.columns
				+common.filter(tables.variables.columns, "nodeId")+{
				variantDataTypeId: tables.variants.columns.dataTypeId+{ i:331 },
				variantArrayDims: tables.variants.columns.arrayDims+{i:332}
			},
			customInsertProc: true
		}
	},
	tables:{
		browseNames:{
			columns:{
				browseId: common.pkSequenced+{ sk:0, i:0 },
				ns: nsIndex+{ i:1 },
				name: common.targetColumns.name+{ i:2 }
			},
			naturalKeys: [ ["ns", "name"] ],
		},
		constructors:{
			columns:{
				nodeId: nodeId+{ pkTable:"node_ids", sk:0, i:0, comment:"parent" },
				browseId: tables.browseNames.columns.browseId+{ sk:1, pkTable:"browse_names", i:1 },
				variantId: tables.variants.columns.variantId+{ pkTable:"variants", i:2 },
			},
			customInsertProc: true
		},
		dataTypes:{
			columns:{
				dataTypeId: nodeId+{ pkTable:"node_ids", length: 256, sk:0, i:1 },
				binaryEncodingId: nodeId+{ pkTable:"node_ids", i:2 },
				memSize: types.uint+{ i:3, default: 16 },
				typeKind: types.uint+{ i:4, default: 6 },
				pointerFree: types.uint+{ i:5, default: 1 },
				overlayable: types.uint+{ i:6, default: 1 },
				description: types.varchar+{ i:7, length: 256, nullable: true },
			}
		},
		dataTypeMemberMap:{
			columns:{
				dataTypeId: nodeId+{ pkTable:"data_types", i:0 },
				memberId: nodeId+{ pkTable:"data_types", i:1 }
			},
			map:{parentId:"data_type_id", childId:"member_id"},
		},
		dataTypeMembers:{
			columns:{
				dataTypeId: nodeId+{ pkTable:"node_ids", i:2 },
				name: types.varchar+{ length: 256, i:1 },
				padding: types.uint8+{ i:3 },
				isArray: types.bit+{ i:4 },
				isOptional: types.bit+{ i:5 }
			}
		},
		nodeIds:{
			columns:{
				nodeId: nodeId+{ i:100, sk: 0, insertable: false },
				ns: nsIndex+{ i:101, default: 0 },
				number: types.uint+{ i:102, nullable: true },
				string: types.varchar+{ i:103, length:256, nullable: true },
				guid: types.guid+{ i:104, nullable: true, length:16 },
				bytes: types.varbinary+{ i:105, length:256, nullable: true },
				namespaceUri: types.varchar+{ i:106, length: 256, nullable: true },
				serverIndex: types.uint+{ i:107, nullable: true },
				isGlobal: types.bit+{ i:108, nullable: true },
			},
			customInsertProc: true,
			naturalKeys: [
				["ns", "number", "string", "guid", "bytes"]
			]
		},
		objects:nodeTableProps+{
			columns: nodeColumns+{
				eventNotifier: types.uint8+{ i:213, nullable: true },
			}
		},
		objectTypes:nodeTableProps+{
			columns: common.filter(nodeColumns, "typeDefId") + {
				isAbstract: types.bit+{ i:213, nullable: true },
			}
		},
		refs:{
			columns:{
				sourceNodeId: nodeId+{ pkTable:"node_ids", sk:0, i:0 },
				targetNodeId: nodeId+{ pkTable:"node_ids", sk:1, i:1 },
				refTypeId: nodeId+{ pkTable:"node_ids", sk:2, i:2 },
				isForward: types.bit+{ i:3, comment: "null=true", nullable: true },
			}
		},
		servers:{
			columns:{
				serverId: common.pkSequenced,
			}+common.targetColumns,
		},
		serverNodeMap:{
			columns:{
				serverId: tables.servers.columns.serverId+{ pkTable: "servers", i:0, sk: 0 },
				nodeId: nodeId+{ pkTable: "node_ids", i:1, sk: 1 },
			},
			map:{parentId:"server_id", childId:"node_id"}
		},
		variables:nodeTableProps+{
			columns: nodeColumns+{
				variantId: tables.variants.columns.variantId+{ pkTable:"variants", i:223, nullable: true },
				dataTypeId: nodeId+{ pkTable: "node_ids", nullable: true, i:224 },
				valueRank: types.int+{ i:225, nullable: true },
				arrayDims: types.varchar+{ i:226, length: 256, nullable: true },
				accessLevel: types.uint8+{ i:227, nullable: true, default: 97 },
				userAccessLevel: types.uint8+{ i:228, nullable: true },
				minimumSamplingInterval: types.float+{ i:229, nullable: true },
				historizing: types.bit+{ i:230, nullable: true }
			}
		},
		variants:{
			columns:{
				variantId: common.pkSequenced+{ i:400 },
				dataTypeId: nodeId+{ pkTable:"node_ids", i:401 },
				arrayDims: types.varchar+{ length: 256, nullable: true, i:402 },
			},
			customInsertProc: true
		},
		variantMembers:{
			columns:{
				variantId: tables.variants.columns.variantId+{ pkTable:"variants", i:0, sk:0 },
				idx: types.uint+{ i:1, sk:1 },
				value: types.varchar+{ i:2, length: 2096, nullable: true },
			}
		},
	}
}