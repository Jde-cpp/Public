local common = import '../../../../../libs/db/config/sqlite-common.libsonnet';
common + {
	local args = self,
	dbServers: {
		scriptPaths: [
			args.repoSourceDir+"/libs/access/config/sql/sqlite",
			args.repoSourceDir+"/apps/AppServer/config/sql/sqlite",
		],
		dataPaths: [
			args.repoSourceDir+"/apps/AppServer/config",
			args.repoSourceDir+"/libs/access/config"
		],
		localhost: common.localhost({
			dbo:{ // n/a for sqlite
				access: common.access(),
				app: common.app(),
			}
		})
	},
}
