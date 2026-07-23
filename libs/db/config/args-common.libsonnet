// Shared preamble for the per-app sqlServer/mysql `config/args/<type>/args.libsonnet` files - the same buildTarget/
// logsDir/repoBuildDir/repoSourceDir/schema cluster that used to be copy-pasted into each one (the sqlite side has
// its own sqlite-common.libsonnet).
//
// Usage: `local common = import '<…>/libs/db/config/args-common.libsonnet'; common + { local args = self, sqlType: …,
// dbServers: {…}, …what varies… }`.  Read the inherited fields back through the merged self - `args.repoBuildDir`,
// `args.repoSourceDir`, `args.schema()` - so an override (none today) would flow through; the fields/method resolve
// against `self`, not the import binding.  `schema()` is hidden (`::`), so files that never call it are unaffected.
{
	local args = self,
	buildTarget: std.extVar("buildTarget"),
	logsDir: std.extVar("logsDir"),
	repoBuildDir: "$(REPO_BUILD_DIR)/"+args.buildTarget,
	repoSourceDir: "$(REPO_SOURCE_DIR)",
	schema():: if args.buildTarget == "release" then "rls" else args.buildTarget,
}
