local common = import 'common-meta.libsonnet';
{
	local tables = self.tables,
	views:{
		connections_ql:{
			columns: {
				connectionId: tables.connections.columns.connectionId,
				//NOT tables.instances.columns.instanceId:  that is pkSequenced, and `sequence:true` forces SKIndex=0
				//(Column.cpp), so it would claim surrogate-key slot 0 alongside connectionId.  getSurrogateKeys keys a
				//flat_map by SKIndex and silently drops the duplicate, leaving both columns answering IsPK() - and
				//TableQL writes every PK column under the literal key "id", so instanceId overwrote the connection id.
				//Here it is plain data, so declare it from the base type with no sequence/sk.
				instanceId: common.types.uint+{ i:1 },
				instanceName: tables.instances.columns.name,
				programName: tables.programs.columns.name,
				hostName: tables.hosts.columns.name,
				created: tables.connections.columns.created,
				deleted: tables.connections.columns.deleted,
				pid: tables.connections.columns.pid
			},
			naturalKeys: [["connection_id"]]
		}
	},
	tables:{
		programs:{
			columns: {
				programId: common.smallSequenced,
				name: common.valuesColumns.name,
				attributes: common.targetColumns.attributes
			},
			customInsertProc: true,
			ops: ["None"]
		},
		instances:{
			columns: {
				instanceId: common.pkSequenced,
				programId: tables.programs.columns.programId+{ pkTable: "programs", i:1 },
				name: common.valuesColumns.name+{ i:2 },
				hostId: tables.hosts.columns.hostId+{ pkTable: "hosts", i:3 },
			},
			customInsertProc: true,
			naturalKeys:[["program_id", "name"]]
		},
		logLevels:{
			columns: {
				levelId: common.types.uint8+{ sk:0, i:0 },
				name: common.valuesColumns.name+{ i:1 }
			},
			naturalKeys:[["name"]],
			ops: ["None"]
		},
		instanceTagLevels:{
			columns: {
				instanceId: tables.instances.columns.instanceId+{ pkTable: "instances", sk:0, i:0 },
				type: common.valuesColumns.name+{ sk:1, i:1 },
				tag: common.types.ulong+{ sk:2, i:2 },
				levelId: tables.logLevels.columns.levelId+{ pkTable: "log_levels", i:3 }
			},
		},
		connections:{
			columns: {
				connectionId: common.pkSequenced,
				instanceId: tables.instances.columns.instanceId+{ pkTable: "instances", i:1 },
				created: common.targetColumns.created+{ i:2 },
				deleted: common.targetColumns.deleted+{ i:3 },
				pid: common.types.ulong+{ i:4 },
			},
			customInsertProc: true,
			qlView: "connections_ql",
			ops: ["None"]
		},
		hosts:{
			columns: {
				hostId: common.smallSequenced,
				name: common.valuesColumns.name
			},
			naturalKeys:[["name"]],
			ops: ["None"]
		},
	}
}