<#
.SYNOPSIS
Authenticode-signs the files named on the command line - the exes and dlls the installer packs, the installer, and, from
makensis's !uninstfinalize hook, the uninstaller.  See README.md ("Signing") beside this script.

.DESCRIPTION
The certificate comes from one of two places, chosen by what is set in the environment:

  Azure Artifact Signing   JDE_SIGN_ENDPOINT (the account's regional endpoint, e.g. https://eus.codesigning.azure.net),
                           JDE_SIGN_ACCOUNT (the account), JDE_SIGN_PROFILE (the certificate profile).  Invoke-ArtifactSigning
                           (the ArtifactSigning module from PSGallery) signs with whatever Azure credential the process has:
                           an `az login` on a dev box, an azure/login OIDC session on a runner.  Timestamped by Microsoft.
  a .pfx                   JDE_SIGN_PFX (and JDE_SIGN_PFX_PASSWORD): signtool from the Windows SDK (JDE_SIGN_TOOL overrides
                           the path), DigiCert's timestamp server.  A self-signed certificate proves the pipeline end to end;
                           it earns no SmartScreen trust.

The settings travel by environment rather than parameters because the uninstaller can only be signed from inside makensis
(!uninstfinalize in OpcHubSetup.nsi), whose child process inherits the environment and gets nothing else.  build-setup.ps1
-Sign calls this for the payload before makensis and for the installer after it.

.EXAMPLE
$env:JDE_SIGN_PFX = 'C:\certs\test.pfx'; .\sign.ps1 R:\clang++\opc-hub\release\bin\Jde.Opc.Hub\Jde.Opc.Hub.exe
#>
[CmdletBinding()]
param(
	[Parameter( Mandatory, ValueFromRemainingArguments )][string[]]$Files #the only parameter - a second one would be positional and eat a path
)
$ErrorActionPreference = 'Stop'
$signTool = if( $env:JDE_SIGN_TOOL ){ $env:JDE_SIGN_TOOL } else { 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe' }
$Files = @( $Files | ForEach-Object { [IO.Path]::GetFullPath( $_ ) } )
foreach( $f in $Files ){ if( -not (Test-Path $f -PathType Leaf) ){ throw "sign.ps1: no such file - $f" } }
$description = 'Jde OpcHub'
$url = 'https://github.com/Jde-cpp/opc-hub'

if( $env:JDE_SIGN_ENDPOINT ){
	foreach( $v in 'JDE_SIGN_ACCOUNT', 'JDE_SIGN_PROFILE' ){ if( -not [Environment]::GetEnvironmentVariable( $v ) ){ throw "sign.ps1: JDE_SIGN_ENDPOINT is set but $v is not" } }
	if( -not (Get-Module -ListAvailable ArtifactSigning) ){ throw 'sign.ps1: the ArtifactSigning module is not installed for this PowerShell host - Install-Module -Name ArtifactSigning -Scope CurrentUser' }
	Import-Module ArtifactSigning
	Write-Host "sign.ps1: Azure Artifact Signing $env:JDE_SIGN_ACCOUNT/$env:JDE_SIGN_PROFILE - $($Files.Count) file(s)"
	Invoke-ArtifactSigning -Endpoint $env:JDE_SIGN_ENDPOINT -CodeSigningAccountName $env:JDE_SIGN_ACCOUNT -CertificateProfileName $env:JDE_SIGN_PROFILE `
		-Files ($Files -join ',') -FileDigest SHA256 -TimestampRfc3161 'http://timestamp.acs.microsoft.com' -TimestampDigest SHA256 `
		-Description $description -DescriptionUrl $url
	$mustBeValid = $true
}
elseif( $env:JDE_SIGN_PFX ){
	if( -not (Test-Path $env:JDE_SIGN_PFX) ){ throw "sign.ps1: JDE_SIGN_PFX not found - $env:JDE_SIGN_PFX" }
	if( -not (Test-Path $signTool) ){ throw "sign.ps1: signtool not found at $signTool - install the Windows SDK or set JDE_SIGN_TOOL" }
	$password = if( $env:JDE_SIGN_PFX_PASSWORD ){ @( '/p', $env:JDE_SIGN_PFX_PASSWORD ) } else { @() }
	Write-Host "sign.ps1: signtool with $env:JDE_SIGN_PFX - $($Files.Count) file(s)"
	& $signTool sign /f $env:JDE_SIGN_PFX @password /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /d $description /du $url @Files
	if( $LASTEXITCODE -ne 0 ){ throw "sign.ps1: signtool failed ($LASTEXITCODE)" }
	$mustBeValid = $false #a self-signed certificate chains to nothing the machine trusts
}
else{ throw 'sign.ps1: nothing to sign with - set JDE_SIGN_ENDPOINT, JDE_SIGN_ACCOUNT and JDE_SIGN_PROFILE (Azure Artifact Signing), or JDE_SIGN_PFX (a .pfx)' }

# whatever the signer reported, the files are the proof
foreach( $f in $Files ){
	$sig = Get-AuthenticodeSignature $f
	if( -not $sig.SignerCertificate ){ throw "sign.ps1: $f carries no signature after signing" }
	if( $mustBeValid -and $sig.Status -ne 'Valid' ){ throw "sign.ps1: $f - signature $($sig.Status): $($sig.StatusMessage)" }
	Write-Host "sign.ps1: $($sig.Status) $($sig.SignerCertificate.Subject) - $f"
}
