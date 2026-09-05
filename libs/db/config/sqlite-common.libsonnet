// Shared skeleton for the per-app `config/args/sqlite/args.libsonnet` files, which otherwise repeat the same
// preamble, the same localhost/driver block, and the same app-schema entries (the AppServer proc MODULE path alone
// appeared 7 times across 4 files, so a change to that layout meant auditing every one of them).
//
// Usage: `local common = import '<…>/libs/db/config/sqlite-common.libsonnet'; common + { …what varies… }`.
// Importers call the helpers through that import binding (`common.localhost(…)`, `common.lib(…)`, …), so `self`
// inside them is *this* object, not the caller's merged `common + {…}`: overriding a base field like buildTarget in
// the `+ {…}` does NOT reach the helpers - they read the ext var bound here.  Only fields read back through the
// merged `self` would see such an override, and no importer does that, so vary behavior by passing helper args.
// `::` members are hidden - helpers and path constants, not config, so they never reach the manifested settings.
local paths = import 'paths-common.libsonnet'; //companyDir/certsDir, shared with args-common so the two can't drift.
paths + {
	local common = self,
	sqlType: "sqlite",
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+common.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	instanceName: paths.instanceNameFor( common.buildTarget ), //see paths-common: the base value there is build-target-free for args/install, which has no ext vars.
	windows:: std.extVar("windows")=="true",
	//windows dlls all land in <buildDir>/bin.  Derived from repoBuildDir, not cwd, so it stays correct wherever the
	//process runs: ctest uses <buildDir>/Testing and direct runs <buildDir>/runtime (both one level down, which the
	//old cwd+"/../bin" relied on), but the soak harness runs each app from a per-run dir outside the build tree.
	binDir:: common.repoBuildDir+"/bin",
	lib( name, linuxDir ):: if common.windows then common.binDir+"/"+name+".dll" else common.repoBuildDir+linuxDir+"/lib"+name+".so",

	// Paths named once.  The proc MODULEs are dlopen'd by the driver from these (see IProcs/sqlite_api.h).
	accessMeta:: common.repoSourceDir+"/libs/access/config/access-meta.jsonnet",
	accessQL:: common.repoSourceDir+"/libs/access/config/access-ql.jsonnet",
	appServerDll:: common.lib( "Jde.DB.Sqlite.AppServer", "/apps/AppServer/config/sql/sqlite" ),

	// The dbServers.localhost block, identical in all six: sqlite has no server, so the credentials are all null and
	// `catalogs.master` is a placeholder whose only real field is the db path (`-arg path=…`, ':memory:' if omitted).
	localhost( schemas ):: {
		driver: common.lib( "Jde.DB.Sqlite", "/libs/db/drivers/sqlite/lib" ),
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
	//opcSchema, not opc: the OpcGateway/OpcServer leaf configs also carry a top-level `opc` data field (the OPC-UA
	//endpoint urn/url), which would shadow a helper named `opc` in the merged object.
	//dynamicLib null, not absent: the OpcServer schema owns no tables and no native procs - its address space lives in
	//NodeSet2 xml - and SqliteDataSource::SetConfig reads the explicit null as "nothing to load" (a missing key still throws).
	opcSchema( extra={} ):: { meta: common.repoSourceDir+"/apps/OpcServer/config/opcServer-meta.jsonnet", prefix: "opc_", dynamicLib: null } + extra,
	gateway( extra={} ):: { meta: common.repoSourceDir+"/apps/OpcGateway/config/opcGateway-meta.jsonnet", prefix: "gateway_",
		dynamicLib: common.lib( "Jde.DB.Sqlite.OpcGateway", "/apps/OpcGateway/config/sql/sqlite" ) } + extra,
}
