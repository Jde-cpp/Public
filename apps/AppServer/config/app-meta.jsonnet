local common = import 'common-meta.libsonnet';
{
	local tables = self.tables,
	views:{
		connections_ql:{
			columns: {
				connectionId: tables.connections.columns.connectionId,
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
			naturalKeys:[["program_id", "name"]],
			ops: ["None"]
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