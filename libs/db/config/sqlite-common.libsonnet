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
{
	local common = self,
	sqlType: "sqlite",
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+common.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	windows:: std.extVar("windows")=="true",
	binDir:: std.extVar("cwd")+"/../bin", //windows dlls all land in <buildDir>/bin; cwd is <buildDir>/Testing (ctest) or <buildDir>/runtime (direct runs).
	lib( name, linuxDir ):: if common.windows then common.binDir+"/"+name+".dll" else common.repoBuildDir+linuxDir+"/lib"+name+".so",
	companyDir:: if common.windows then "$(ProgramData)/Jde-Cpp" else "$(HOME)/.Jde-Cpp", //per-OS company data root the apps write certs under.
	certsDir( product ):: common.companyDir+"/"+product+"/ssl/certs",
	opcTestsProduct:: if common.windows then "OpcTests" else "Tests.Opc", //the opc test client's ProductName: windows takes the .rc value, linux the hardcoded one.

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
	opcSchema( extra={} ):: { meta: common.repoSourceDir+"/apps/OpcServer/config/opcServer-meta.jsonnet", prefix: "opc_",
		dynamicLib: common.lib( "Jde.DB.Sqlite.OpcServer", "/apps/OpcServer/config/sql/sqlite" ) } + extra,
	gateway( extra={} ):: { meta: common.repoSourceDir+"/apps/OpcGateway/config/opcGateway-meta.jsonnet", prefix: "gateway_",
		dynamicLib: common.lib( "Jde.DB.Sqlite.OpcGateway", "/apps/OpcGateway/config/sql/sqlite" ) } + extra,
}
