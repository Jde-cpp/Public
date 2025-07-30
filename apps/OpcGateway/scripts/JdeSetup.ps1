$Server = "localhost"
$AppDB = "jde"

Write-Host "Creating Databases..."
Invoke-Sqlcmd -Query "if not exists(select * from sys.databases where name = '$($AppDB)') create database $($AppDB)" -ServerInstance "$($Server)"
Write-Host "Granting DB Owner to System account..."
Invoke-Sqlcmd -Query "use $($AppDB);exec sp_addrolemember 'db_owner', 'NT AUTHORITY\System'; ALTER USER [NT AUTHORITY\System] WITH DEFAULT_SCHEMA=dbo;"

Write-Host "Creating ODBC Datasources..."
if ( (Get-OdbcDsn -Name "jde" -ErrorAction Ignore) -eq $null){
  Add-OdbcDsn -Name "jde" -DriverName "ODBC Driver 17 for SQL Server" -Platform "64-bit" -DsnType "System" -SetPropertyValue @("SERVER=$($SERVER)", "Trusted_Connection=Yes", "DATABASE=$($AppDB)")
}
Write-Host "Registering Services..."
& "$Env:Programfiles\Jde-cpp\OpcGateway\Jde.Opc.Gateway.exe"  -install
& "$Env:Programfiles\Jde-cpp\AppServer\Jde.App.Server.exe"  -install

Read-Host -Prompt "Press Enter to exit"