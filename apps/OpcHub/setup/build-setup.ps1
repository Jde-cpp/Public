<#
.SYNOPSIS
Builds OpcHubSetup-<version>.exe from the release build tree - see README.md beside this script.

.DESCRIPTION
Resolves the inputs the NSIS script takes as /D defines (build tree, Angular dist, UA-Nodeset clone, VC++ redistributable,
version), checks they exist, and runs makensis.  Optionally signs the result.

.EXAMPLE
.\build-setup.ps1                       # defaults: $env:JDE_RBUILD_DIR\clang++\<repo dir>\release, the repo's web dist, git describe
.\build-setup.ps1 -SkipWeb -Version 1.0 # no Web UI component
#>
[CmdletBinding()]
param(
	[string]$BuildDir,                   # the release build tree: bin\Jde.Opc.Hub\, bin\Jde.Opc.Server\, bin\Jde.DB.Sqlite*.dll
	[string]$WebDist,                    # ng build output - web\opc\my-workspace\dist\my-workspace\browser
	[switch]$SkipWeb,                    # omit the Web UI component
	[string]$UaNodeSets = $env:UA_NODE_SETS, # OPCFoundation/UA-Nodeset clone (DI/IA for the OpcServer)
	[string]$VcRedist = 'C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Redist\MSVC\v145\vc_redist.x64.exe',
	[string]$Version,                    # default: git describe --tags --always
	[string]$OutDir,                     # default: <BuildDir>\setup - outside the repo
	[string]$MakeNsis = 'C:\Program Files (x86)\NSIS\makensis.exe',
	[switch]$Sign,
	[string]$PfxPath,
	[string]$SignTool = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe'
)
$ErrorActionPreference = 'Stop'
$setupDir = $PSScriptRoot
$repo = (Resolve-Path (Join-Path $setupDir '..\..\..')).Path

if( -not $BuildDir ){
	# the win-clang-release-jde preset's binaryDir is $env:JDE_BUILD_DIR\clang++-jde\release; the release tree actually
	# built here is $env:JDE_RBUILD_DIR\clang++\<repo dir>\release - so no preset lookup, an explicit default.
	# The <repo dir> segment is the checkout's own name, the same basename rule buildFunctions.sh and the jde extension use.
	$rbuild = if( $env:JDE_RBUILD_DIR ){ $env:JDE_RBUILD_DIR } else { 'R:\' }
	$BuildDir = Join-Path $rbuild ('clang++\{0}\release' -f (Split-Path $repo -Leaf))
}
if( -not $WebDist ){ $WebDist = Join-Path $repo 'web\opc\my-workspace\dist\my-workspace\browser' }
if( -not $UaNodeSets ){ $UaNodeSets = 'C:\Users\duffyj\source\repos\libs\UA-Nodeset' }
if( -not $OutDir ){ $OutDir = Join-Path $BuildDir 'setup' }
# Native separators for everything handed to makensis: its compile-time `!if /FileExists` does not take a forward-slash
# path (the CI run passed C:/jde/vc_redist.x64.exe and the runtime silently went unbundled), and GetFullPath also
# resolves a relative dir against the caller's cwd rather than the script's.
$BuildDir = [IO.Path]::GetFullPath( $BuildDir )
$WebDist = [IO.Path]::GetFullPath( $WebDist )
$UaNodeSets = [IO.Path]::GetFullPath( $UaNodeSets )
$OutDir = [IO.Path]::GetFullPath( $OutDir )
if( $VcRedist ){ $VcRedist = [IO.Path]::GetFullPath( $VcRedist ) }

foreach( $f in 'bin\Jde.Opc.Hub\Jde.Opc.Hub.exe', 'bin\Jde.Opc.Server\Jde.Opc.Server.exe', 'bin\Jde.DB.Sqlite.dll', 'bin\sqlite3.dll', 'bin\Jde.DB.Sqlite.AppServer.dll', 'bin\Jde.DB.Sqlite.OpcGateway.dll' ){
	if( -not (Test-Path (Join-Path $BuildDir $f)) ){ throw "missing $f under $BuildDir - build Jde.Opc.Hub, Jde.Opc.Server, Jde.DB.Sqlite, Jde.DB.Sqlite.AppServer and Jde.DB.Sqlite.OpcGateway in the release tree first, or pass -BuildDir" }
}
if( -not $SkipWeb -and -not (Test-Path (Join-Path $WebDist 'index.html')) ){ throw "no index.html under $WebDist - run web/opc/scripts/setup.sh (ng build), pass -WebDist, or -SkipWeb" }
foreach( $f in 'DI\Opc.Ua.Di.NodeSet2.xml', 'IA\Opc.Ua.IA.NodeSet2.xml', 'IA\Opc.Ua.IA.NodeSet2.examples.xml' ){
	if( -not (Test-Path (Join-Path $UaNodeSets $f)) ){ throw "missing $f under $UaNodeSets - clone https://github.com/OPCFoundation/UA-Nodeset or pass -UaNodeSets" }
}
if( -not (Test-Path $MakeNsis) ){ throw "makensis not found at $MakeNsis - install NSIS 3.x or pass -MakeNsis" }
if( -not (Test-Path $VcRedist) ){ Write-Warning "vc_redist.x64.exe not found at $VcRedist - the installer will not bundle the Visual C++ runtime"; $VcRedist = '' }

if( -not $Version ){ $Version = (& git -C $repo describe --tags --always).Trim() }
# VIProductVersion needs four 16-bit numbers: yyyy.M.d.N from a `yyyy.MM.dd[-N-gsha]` describe, else 0.0.0.0
$vi = '0.0.0.0'
if( $Version -match '^(\d{4})\.(\d{1,2})\.(\d{1,2})(?:-(\d+)-g[0-9a-f]+)?$' ){
	$n = if( $Matches[4] ){ [int]$Matches[4] } else { 0 }
	$vi = "$([int]$Matches[1]).$([int]$Matches[2]).$([int]$Matches[3]).$n"
}
New-Item -ItemType Directory -Force $OutDir | Out-Null

$defs = @( "/DBUILD_DIR=$BuildDir", "/DWEB_DIST=$WebDist", "/DUA_NODE_SETS=$UaNodeSets", "/DVERSION=$Version", "/DVI_VERSION=$vi", "/DOUT_DIR=$OutDir" )
if( $VcRedist ){ $defs += "/DVC_REDIST=$VcRedist" }
if( $SkipWeb ){ $defs += '/DSKIP_WEB' }
Write-Host "makensis $($defs -join ' ')"
& $MakeNsis /V2 @defs (Join-Path $setupDir 'OpcHubSetup.nsi')
if( $LASTEXITCODE -ne 0 ){ throw "makensis failed ($LASTEXITCODE)" }
$out = Join-Path $OutDir "OpcHubSetup-$Version.exe"
if( -not (Test-Path $out) ){ throw "makensis succeeded but $out is missing" }

if( $Sign ){
	if( -not $PfxPath ){ throw '-Sign needs -PfxPath' }
	if( -not (Test-Path $SignTool) ){ throw "signtool not found at $SignTool" }
	& $SignTool sign /f $PfxPath /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 $out
	if( $LASTEXITCODE -ne 0 ){ throw "signtool failed ($LASTEXITCODE)" }
}
Write-Host "built $out ($([math]::Round((Get-Item $out).Length/1MB,1)) MB)"
