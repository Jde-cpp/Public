// The on-disk data root the apps read and write cert material under, named once for every args file - sqlite-common
// and args-common (sqlServer/mysql) both merge this in, so their importers inherit `companyDir`/`certsDir` and no
// args file spells the root literally.
//
// `$(ProgramData)` resolves on both platforms, so this needs no per-OS branch: windows takes the env var
// (C:\ProgramData), linux has none and falls through to `Process::ProgramDataFolder()` in settings.cpp's expander
// ($XDG_CONFIG_HOME, else $HOME/.config).  Keep it branch-free - these paths drifted out of sync with the code once
// already, when the linux root moved from $HOME/.Jde-Cpp to $XDG_CONFIG_HOME/Jde-Cpp and the configs stayed behind.
//
// The trailing company component matches `Process::CompanyName()` exactly ("Jde-Cpp"); linux filesystems are
// case-sensitive, so a "jde-cpp" spelling here silently anchors nothing.
{
	local paths = self,
	companyDir:: "$(ProgramData)/Jde-Cpp",
	certsDir( product ):: paths.companyDir+"/"+product+"/ssl/certs",
}
