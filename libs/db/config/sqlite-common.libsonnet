// Shared skeleton for the per-app `config/args/sqlite/args.libsonnet` files, which otherwise repeat the same
// preamble, the same localhost/driver block, and the same app-schema entries (the AppServer proc MODULE path alone
// appeared 7 times across 4 files, so a change to that layout meant auditing every one of them).
//
// Usage: `local common = import '<…>/libs/db/config/sqlite-common.libsonnet'; common + { …what varies… }`.
// `local common = self` + jsonnet late binding means the helpers re-resolve against the *merged* object, so a caller
// overriding e.g. buildTarget flows through repoBuildDir and every path built from it.
// `::` members are hidden - helpers and path constants, not config, so they never reach the manifested settings.
{
	local common = self,
	sqlType: "sqlite",
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+common.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",

	// Paths named once.  The proc MODULEs are dlopen'd by the driver from these (see IProcs/sqlite_api.h).
	accessMeta:: common.repoSourceDir+"/libs/access/config/access-meta.jsonnet",
	accessQL:: common.repoSourceDir+"/libs/access/config/access-ql.jsonnet",
	appServerDll:: common.repoBuildDir+"/apps/AppServer/config/sql/sqlite/libJde.DB.Sqlite.AppServer.so",

	// The dbServers.localhost block, identical in all six: sqlite has no server, so the credentials are all null and
	// `catalogs.master` is a placeholder whose only real field is the db path (`-arg path=…`, ':memory:' if omitted).
	localhost( schemas ):: {
		driver: common.repoBuildDir+"/libs/db/drivers/sqlite/lib/libJde.DB.Sqlite.so",
		connectionString: null,
		username: null,
		password: null,
		schema: null,
		catalogs: {
			master: { // n/a for sqlite
				path: std.extVar("path"),
				schemas: schemas
			}
		}
	},

	// The app-schema entries, each defined once; `extra` adds or overrides.  The `access` schema has two reduced
	// forms in use - see the `_appServer`/`_access` mounts in the OpcServer/OpcGateway app args - which are written
	// out there rather than expressed as subtractions here, but still share the path constants above.
	access( extra={} ):: { meta: common.accessMeta, ql: common.accessQL, prefix: "access_", dynamicLib: common.appServerDll } + extra,
	app( extra={} ):: { meta: common.repoSourceDir+"/apps/AppServer/config/app-meta.jsonnet", prefix: "app_", dynamicLib: common.appServerDll } + extra,
	opc( extra={} ):: { meta: common.repoSourceDir+"/apps/OpcServer/config/opcServer-meta.jsonnet", prefix: "opc_",
		dynamicLib: common.repoBuildDir+"/apps/OpcServer/config/sql/sqlite/libJde.DB.Sqlite.OpcServer.so" } + extra,
	gateway( extra={} ):: { meta: common.repoSourceDir+"/apps/OpcGateway/config/opcGateway-meta.jsonnet", prefix: "gateway_",
		dynamicLib: common.repoBuildDir+"/apps/OpcGateway/config/sql/sqlite/libJde.DB.Sqlite.OpcGateway.so" } + extra,
}
