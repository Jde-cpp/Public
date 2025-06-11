local common = import 'common-meta.libsonnet';
{
	local types = common.types,
	local tables = self.tables,
	local nodeId = types.ulong,
	local nodeIdNull = nodeId+{nullable: true},
	local nsIndex = types.uint16,
	views:{
		serverNodes:{
			comment: "Nodes of a server",
			columns:{
				serverId: types.uint+{ i:200 },
				serverName: types.varchar+{ i:201 },
				serverTarget: types.varchar+{ i:202 },
				serverCreated: types.dateTime+{ i:203 },
				serverUpdated: types.dateTime+{ i:204 },
				serverDeleted: types.dateTime+{ i:205 },
				serverDescription: types.varchar+{ i:206 }
			}+tables.nodeIds.columns+tables.nodes.columns,
		}
	},
	tables:{
		dataTypes: {
			columns: {
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
			columns: {
				dataTypeId: nodeId+{ pkTable:"data_types", i:0 },
				memberId: nodeId+{ pkTable:"data_types", i:1 }
			},
			map: {parentId:"data_type_id", childId:"member_id"},
		},
		dataTypeMembers: {
			columns:{
				name: types.varchar+{ length: 256, i:1 },
				dataTypeId: nodeId+{ pkTable:"data_types", i:2 },
				padding: types.uint8+{ i:3 },
				isArray: types.bit+{ i:4 },
				isOptional: types.bit+{ i:5 }
			}
		},
		nodeIds: {
			columns: {
				nodeId: nodeId+{ i:0, sk: 0 },
				ns: nsIndex+{ i:1 },
				number: types.uint+{ i:2, nullable: true },
				string: types.varchar+{ i:3, length:256, nullable: true },
				guid: types.guid+{ i:4, nullable: true, length:16 },
				bytes: types.varbinary+{ i:5, length:256, nullable: true },
				namespaceUri: types.varchar+{ i:6, length: 256, nullable: true },
				serverIndex: types.uint+{ i:7, nullable: true },
				isGlobal: types.bit+{ i:8, nullable: true },
			},
			customInsertProc: true,
			naturalKeys: [
				["ns", "number", "string", "guid", "bytes"],
				["server_index", "namespace_uri"]
			]
		},
		nodes: {
			columns: {
				nodeId: nodeId+{ pkTable:"node_ids", sk: 0, i:100 },
				parentNodeId: nodeIdNull+{ pkTable:"node_ids", i:101 },
				referenceTypeId: nodeId+{ pkTable:"node_ids", i:102 },
				typeDefinitionId: nodeId+{ pkTable:"node_ids", i:103 },
				objectAttrId:tables.objectAttrs.columns.objectAttrId+{ pkTable: "object_attrs", i:104 },
				typeAttrId: tables.typeAttrs.columns.typeAttrId+{ pkTable: "type_attrs", i:105 },
				variableAttrId: tables.variableAttrs.columns.variableAttrId+{ pkTable: "variable_attrs", i:106 },
				name: common.targetColumns.name+{ i:107 },
				created: common.targetColumns.created+{ i:108 },
				deleted: common.targetColumns.deleted+{ i:109 },
				updated: common.targetColumns.updated+{ i:110 }
			}
		},
		nodeBrowseNames: {
			columns: {
				nodeId: nodeId+{ pkTable:"node_ids", i:0 },
				ns: nsIndex+{ i:1 },
				browseName: common.targetColumns.name+{ i:2 }
			}
		},
		typeAttrs:{
			columns: {
				typeAttrId: common.pkSequenced,
				name: types.varchar+{ length: common.valuesColumns.name.length, i:1 },
				description: types.varchar+{ length: common.targetColumns.description.length, nullable: true, i:2 },
				writeMask: types.uint+{ i:3 },
				userWriteMask: types.uint+{ i:4 },
				isAbstract: types.bit+{ i:5 }
			}
		},
		objectAttrs:{
			columns: {
				objectAttrId: common.pkSequenced+{ sk:0 },
				specified: types.uint+{ i:1 },
				name: types.varchar+{ length: common.valuesColumns.name.length, i:2 },
				description: types.varchar+{ length: 2048, nullable: true, i:3 },
				writeMask: types.uint+{ i:4 },
				userWriteMask: types.uint+{ i:5 },
				eventNotifier: types.uint8+{ i:6 },
			}
		},
		reference_node_map:{
			columns: {
				sourceNodeId: nodeId+{ pkTable:"node_ids", sk:0, i:0 },
				targetNodeId: nodeId+{ pkTable:"node_ids", sk:1, i:1 },
				referenceTypeId: nodeId+{ pkTable:"node_ids", sk:2, i:2 },
				isForward: types.bit+{ i:3, comment: "null=true" },
			}
		},
		servers: {
			columns: {
				serverId: common.pkSequenced,
			}+common.targetColumns,
		},
		serverNodeMap: {
			columns: {
				serverId: tables.servers.columns.serverId+{ pkTable: "servers", i:0, sk: 0 },
				nodeId: nodeId+{ pkTable: "nodes", i:1, sk: 1 },
			},
			map: {parentId:"server_id", childId:"node_id"}
		},
		variableAttrs:{
			columns: {
				variableAttrId: common.pkSequenced,
				specified: types.uint+{ i:101 },
				name: common.targetColumns.name+{ i:102 },
				description: common.targetColumns.description+{ i:103 },
				writeMask: types.uint+{ i:104 },
				userWriteMask: types.uint+{ i:105 },
				variantId: tables.variants.columns.variantId+{ pkTable:"variants", i:106 },
				dataType: nodeId+{ pkTable:"data_types", i:107 },
				valueRank: types.int+{ i:108 },
				arrayDims: types.varchar+{ length: 256, nullable: true, i:109 },
				accessLevel: types.uint8+{ i:110 },
				userAccessLevel: types.uint8+{ i:111 },
				minimumSamplingInterval: types.float+{ i:112 },
				historizing: types.bit+{ i:113 }
			}
		},
		variants:{
			columns: {
				variantId: common.pkSequenced,
				dataTypeId: nodeId+{ pkTable:"data_types", i:1 },
				arrayDims: types.varchar+{ length: 256, nullable: true, i:2 },
			}
		},
		variantMembers: {
			columns: {
				variantId: tables.variants.columns.variantId+{ pkTable:"variants", i:0, sk:0 },
				idx: types.uint+{ i:1, sk:1 },
				value: types.varchar+{ i:2, length: 2048, nullable: true },
			}
		},
	}
}