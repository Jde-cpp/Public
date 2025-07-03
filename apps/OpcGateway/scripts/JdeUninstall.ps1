Write-Host "Removing Services..."
if( Test-Path -Path $Env:Programfiles\Jde-cpp\OpcGateway\Jde.Opc.Gateway.exe ) {
	& "$Env:Programfiles\Jde-cpp\OpcGateway\Jde.Opc.Gateway.exe"  -uninstall
}
if( Test-Path -Path $Env:Programfiles\Jde-cpp\AppServer\Jde.App.Server.exe ) {
	& "$Env:Programfiles\Jde-cpp\AppServer\Jde.App.Server.exe"  -uninstall
}

if ( (Get-OdbcDsn -Name "jde" -ErrorAction Ignore) -ne $null){
  Remove-OdbcDsn -Name "jde" -Platform "64-bit" -DsnType "System"
}
[Environment]::SetEnvironmentVariable('Jde_App_Connection', $null, 'Machine')
if( Test-Path -Path $Env:PROGRAMFILES/jde-cpp ) {
  Remove-Item -LiteralPath $Env:PROGRAMFILES/jde-cpp -Force -Recurse
}
if( Test-Path -Path $Env:ProgramData/jde-cpp ) {
  Remove-Item -LiteralPath $Env:ProgramData/jde-cpp -Force -Recurse
}
if( Test-Path -Path "$([Environment]::GetFolderPath("CommonDesktopDirectory"))/JdeSetup.ps1" ) {
  Remove-Item -LiteralPath "$([Environment]::GetFolderPath("CommonDesktopDirectory"))/JdeSetup.ps1"
}


$Server = "localhost"
$AppDB = "jde"
#Invoke-Sqlcmd -Query "if exists(select * from sys.databases where name = '$($AppDB)') drop database $($AppDB)" -ServerInstance "$($Server)"
Read-Host -Prompt "Press Enter to exit"
