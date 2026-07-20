local common = import '../../../../../../libs/db/config/sqlite-common.libsonnet';
common + {
	dbServers: {
		localhost: common.localhost({
			dbo:{ // n/a for sqlite
				access: common.access(),
			}
		})
	},
}
