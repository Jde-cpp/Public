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
				url: common.types.varchar+{length:2048, i:104}
			}+common.targetColumns+{
				//target is the connection's identity everywhere but this table: the url segment, the key the live UAClient sits
				//under (UM), the sessions join key.  The update path skips a non-updateable column, so a rename is dropped rather
				//than leaving the running gateway keyed by a name the row no longer has.  Only here - users/roles/groups may rename.
				target: common.targetColumns.target+{updateable:false}
			},
			customInsertProc:true,
			naturalKeys: common.targetNKs
		},
	}
}