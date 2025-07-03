local common = import 'common-meta.libsonnet';
{
	local tables = self.tables,
	tables:{
		clients:{
			columns: {
				client_id: common.pkSequenced,
				is_default: common.types.bit+{i:101, default:false},
				certificate_uri: common.types.varchar+{nullable:true, length:2048,i:102},
				url: common.types.varchar+{nullable:true, length:2048, i:103}
			}+common.targetColumns,
			customInsertProc:true,
			naturalKeys: common.targetNKs
		},
	}
}