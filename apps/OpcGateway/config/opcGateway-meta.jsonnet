local common = import 'common-meta.libsonnet';
{
	local tables = self.tables,
	tables:{
		server_connections:{
			columns: {
				server_connection_id: common.pkSequenced,
				is_default: common.types.bit+{i:101, default:false},
				default_browse_ns: common.types.uint16+{i:102, nullable:true},
				certificate_uri: common.types.varchar+{nullable:true, length:2048,i:103},
				url: common.types.varchar+{nullable:true, length:2048, i:104}
			}+common.targetColumns,
			customInsertProc:true,
			naturalKeys: common.targetNKs
		},
	}
}