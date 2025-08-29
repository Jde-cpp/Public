local common = import 'common-meta.libsonnet';

local crc64 = common.types.ulong+{ i:0 };
{
	local tables = self.tables,
	tables:{
		apps:{
			columns: {
				appId: common.smallSequenced,
				name: common.valuesColumns.name,
				attributes: common.targetColumns.attributes
			},
			customInsertProc: true,
			ops: ["None"]
		},
		appInstances:{
			columns: {
				appInstanceId: common.pkSequenced,
				appId: tables.apps.columns.appId+{ pkTable: "apps", i:1 },
				endTime: common.types.dateTime+{ i:2, nullable:true },
				hostId: tables.hosts.columns.hostId+{ pkTable: "hosts", i:3 },
				pid: common.types.ulong+{ i:4 },
				startTime: common.types.dateTime+{ i:5 }
			},
			customInsertProc: true,
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
		sourceFiles:{
			columns: {
				sourceFileId: crc64+{sk:0},
				path: common.valuesColumns.name
			},
			naturalKeys:[["path"]],
			ops: ["None"]
		},
		sourceFunctions:{
			columns: {
				sourceFunctionId: crc64+{sk:0},
				name: common.valuesColumns.name
			},
			naturalKeys:[["name"]],
			ops: ["None"]
		},
		messages:{
			columns: {
				messageId: crc64+{sk:0},
				text: common.types.varchar+{ length: 4096, i:1 }
			},
			naturalKeys::[["text"]],
			ops: ["None"]
		},
		args:{
			columns: {
				argId: crc64+{sk:0},
				text: common.types.varchar+{ length: 4096, i:1 },
			},
			naturalKeys::[["text"]],
			ops: ["None"]
		},
		entryArgMap:{
			columns: {
				entryId: tables.entries.columns.entryId+{ pkTable: "entries", sk:0, i:0 },
				arg_index: common.types.uint8+{ sk:1, i:2, },
				argId: tables.args.columns.argId+{ pkTable: "args", i:3 }
			},
			map: {parentId:"entry_id", childId:"arg_id"},
			naturalKeys:: [["entry_id", "arg_index"]],
			ops: ["None"]
		},
		entries:{
			columns: {
				entryId: common.longSequenced,
				appInstanceId: tables.appInstances.columns.appInstanceId+{ nullable:true, pkTable: "app_instances", i:1 },
				messageId: tables.messages.columns.messageId+{ pkTable: "messages", nullable: true, i:2 },
				lineNumber: common.types.uint16+{ nullable:true, i:3 },
				severity: tables.severities.columns.severityId+{ i:4 },
				sourceFileId: tables.sourceFiles.columns.sourceFileId+{ nullable:true, pkTable: "source_files", i:5 },
				sourceFunctionId: tables.sourceFunctions.columns.sourceFunctionId+{ nullable:true, pkTable: "source_functions", i:6 },
				tags: tables.tags.columns.tagId+{nullable:true, i:7 },
				time: common.types.dateTime+{ i:8 },
				threadId: common.types.ulong+{ i:9 },
				userId: common.types.uint+{ i:10 }
			},
			ops: ["None"]
		},
		tags:{
			columns: {
				tagId: common.types.long+{ sk:0, i:0 },
				name: common.valuesColumns.name
			},
			ops: ["None"]
		},
		severities:{
			columns: {
				severityId: common.types.uint8+{ sk:0, i:0 },
				name: common.types.varchar+{ length: 11, i:1 }
			},
			ops: ["None"]
		},
		sessions:{
			columns: {
				id: common.types.varchar+{ length: 255, sk:0, i:0 },
				userId: common.types.uint+{ i:1 },
				attributes: common.targetColumns.attributes+{ i:2 },
				created: common.targetColumns.created+{ i:3 },
				updated: common.types.dateTime+{ i:4 },
				deleted: common.types.dateTime+{ i:5 }
			},
			ops: ["None"]
		}
	}
}