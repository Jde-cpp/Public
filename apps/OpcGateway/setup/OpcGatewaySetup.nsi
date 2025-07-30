;signtool sign /f C:\Users\duffyj\source\repos\jde\Private\certs\jde-cpp.pfx /fd SHA256 Iot.exe
Outfile "OpcGatewaySetup.exe"

InstallDir  $PROGRAMFILES64\jde-cpp

Section
  !include "winmessages.nsh"
  !define env_hklm 'HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"'
  WriteRegExpandStr ${env_hklm} Jde_Connection DSN=jde
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
SectionEnd
Section
  SetOutPath $INSTDIR\OpcGateway
  File /r Z:\build\msvc\jde\apps\IotWebsocket\bin\RelWithDebInfo\*.exe
SectionEnd
Section
  SetOutPath $INSTDIR\OpcGateway
  File /r Z:\build\msvc\jde\apps\IotWebsocket\bin\RelWithDebInfo\*.dll
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\OpcGateway
  File C:\Users\duffyj\source\repos\jde\IotWebsocket\config\*sonnet
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\OpcGateway
  File C:\Users\duffyj\source\repos\jde\IotWebsocket\config\win\install\args.libsonnet
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\OpcGateway\sql
  File /r C:\Users\duffyj\source\repos\jde\IotWebsocket\config\sql\sqlServer\*.sql
SectionEnd
Section
  SetOutPath $INSTDIR\AppServer
  File /r Z:\build\msvc\jde\apps\AppServer\bin\RelWithDebInfo\*.exe
SectionEnd
Section
  SetOutPath $INSTDIR\AppServer
  File /r Z:\build\msvc\jde\apps\AppServer\bin\RelWithDebInfo\*.dll
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\AppServer
  File C:\Users\duffyj\source\repos\jde\AppServer\config\*sonnet
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\AppServer
  File C:\Users\duffyj\source\repos\jde\AppServer\config\win\install\args.libsonnet
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\AppServer
  File C:\Users\duffyj\source\repos\jde\Public\libs\access\config\*.jsonnet
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\AppServer\sql
  File /r C:\Users\duffyj\source\repos\jde\AppServer\config\*.mutation
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\AppServer\sql
  File /r C:\Users\duffyj\source\repos\jde\AppServer\config\sql\sqlServer\*.sql
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\AppServer\sql
  File /oname=access.mutation C:\Users\duffyj\source\repos\jde\Public\libs\access\config\release.mutation
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\AppServer\sql
  File C:\Users\duffyj\source\repos\jde\Public\libs\access\config\sql\sqlServer\*.sql
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $LOCALAPPDATA\jde-cpp\OpcWeb
  File /r C:\Users\duffyj\source\repos\jde\web\IotSite\my-workspace\dist\my-workspace\browser\**.*
SectionEnd
Section
  SetShellVarContext all
  SetOutPath $DESKTOP
  File /r C:\Users\duffyj\source\repos\jde\IotWebsocket\scripts\*.ps1
SectionEnd
;Section "Visual Studio Runtime"
;  SetOutPath "$INSTDIR"
;  File "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC\14.42.34433\vc_redist.x64.exe"
;  ExecWait "$INSTDIR\vcredist_x64.exe"
;  Delete "$INSTDIR\vcredist_x64.exe"
;SectionEnd
;Section "Uninstall"
  ;xDelete $INSTDIR\OpcGateway\*.*
  ;xDelete $INSTDIR\AppServer\*.*
;  RMDir $INSTDIR\OpcGateway
;  RMDir $INSTDIR\AppServer
;  RMDir $LOCALAPPDATA\jde-cpp\AppServer
;  RMDir $LOCALAPPDATA\jde-cpp\OpcGateway
;  Delete $DESKTOP\JdeDBSetup.ps1
;  Delete $DESKTOP\OpcGatewayWeb.bat
;SectionEnd