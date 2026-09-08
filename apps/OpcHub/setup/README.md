# Jde OpcHub - Windows installer

`OpcHubSetup.nsi` builds `OpcHubSetup-<version>.exe` (NSIS 3.11, Modern UI 2, `MultiUser.nsh`).  It installs the hub, optionally
the OPC UA server and the Web UI files, as Windows services or as a per-user install without administrator rights, with
sqlite as the database.

## Building the installer

Prerequisites on the build machine:

| what | where |
|---|---|
| NSIS 3.x | `C:\Program Files (x86)\NSIS` (`-MakeNsis` otherwise) |
| the release build tree | `$env:JDE_RBUILD_DIR\clang++\<repo dir>\release` (`-BuildDir`): `bin\Jde.Opc.Hub\`, `bin\Jde.Opc.Server\` and, in `bin\`, `Jde.DB.Sqlite.dll`, `sqlite3.dll`, `Jde.DB.Sqlite.AppServer.dll`, `Jde.DB.Sqlite.OpcGateway.dll` |
| the Angular site | `web\opc\my-workspace\dist\my-workspace\browser` - `web/opc/scripts/setup.sh` runs `ng build` (`-WebDist`, or `-SkipWeb`) |
| [OPCFoundation/UA-Nodeset](https://github.com/OPCFoundation/UA-Nodeset) | `$env:UA_NODE_SETS` (`-UaNodeSets`) - DI/IA nodesets for the OpcServer |
| `vc_redist.x64.exe` | the VS install's `VC\Redist\MSVC\v14x\` (`-VcRedist`); bundled for the all-users mode, skipped with a warning if missing |

```powershell
.\build-setup.ps1                              # -> <BuildDir>\setup\OpcHubSetup-<git describe>.exe
.\build-setup.ps1 -Version 2026.09.08 -SkipWeb
.\build-setup.ps1 -Sign -PfxPath <cert.pfx>    # signtool from the Windows 10 SDK
```

Every input is a `/D` define of the script, so `makensis /DBUILD_DIR=… OpcHubSetup.nsi` works without the wrapper.

CI: the Win2025 workflow (`.github/workflows/win2025-build.yml`) runs `build-setup.ps1` after its release build - the
nodesets and `vc_redist.x64.exe` are downloaded, the Web UI comes from the workflow's `web` job (an `ubuntu-latest` run of
`web/opc/scripts/setup.sh`) - and uploads `OpcHubSetup-<version>.exe` as an artifact; a push of a `yyyy.MM.dd` tag runs it
too and publishes the installer as that tag's GitHub release.

## Install modes

| | All users | Current user |
|---|---|---|
| rights | administrator (UAC prompt) | none - a standard user never sees a prompt; an administrator sees one and may still pick this mode |
| program dir | `C:\Program Files\Jde-Cpp` | `%LOCALAPPDATA%\Programs\Jde-Cpp` |
| how the products run | Windows services `Jde.OpcHub`, `Jde.OpcServer` (auto start; `net start`/`net stop`) | Start Menu folder `Jde-Cpp`: a shortcut per product, each a console window (`-c`); optional "Start at logon" component (HKCU Run) |
| VC++ 2015-2022 x64 runtime | installed when missing | must be present already (installing it needs administrator rights) |
| Add/Remove Programs | HKLM | HKCU (`Jde OpcHub (current user)`) |
| data | `C:\ProgramData\Jde-Cpp\<Product>` in both modes - the apps hardcode it (`Process::ProgramDataFolder()`, `libs/db/config/paths-common.libsonnet`).  A standard user can create the tree and owns it; one created by an all-users install is read-only to them, so the installer refuses the current-user mode in that case. | |

Silent: `OpcHubSetup-<v>.exe /S /AllUsers` or `/CurrentUser`, `/OpcServer` to add the OPC UA Server component (there is no
components page to pick it on), `/D=<dir>` for the program dir.

## Components

| component | section | ships |
|---|---|---|
| OPC Hub (`Jde.OpcHub`) | required | `Jde.Opc.Hub.exe` - the AppServer and the OpcGateway in one process, port 1967 |
| OPC UA Server (`Jde.OpcServer`) | optional, off | `Jde.Opc.Server.exe` - opc.tcp 4840, http 1970, DI/IA nodesets + the pumps demo address space; logs in to the hub with its certificate |
| Web UI files | optional, on | the Angular site + `web.config` under `<program dir>\Web`, for an IIS site (IIS is configured by hand - `apps/OpcGateway/README.md`) |
| Start at logon | current-user only | HKCU Run entries for the selected products |

## Installed layout

```
<program dir>                                            C:\Program Files\Jde-Cpp  |  %LOCALAPPDATA%\Programs\Jde-Cpp
  OpcHub\     Jde.Opc.Hub.exe Jde.dll Jde.DB.dll fmt.dll z.dll libcrypto-3-x64.dll libssl-3-x64.dll
              Jde.DB.Sqlite.dll sqlite3.dll Jde.DB.Sqlite.AppServer.dll Jde.DB.Sqlite.OpcGateway.dll
  OpcServer\  Jde.Opc.Server.exe + the same + libxml2.dll, Jde.DB.Sqlite.dll sqlite3.dll
  Web\        the Angular site + web.config
  Uninstall.exe
C:\ProgramData\Jde-Cpp
  config\                                                settings mirror - repo layout, so the configs' relative imports keep working
    apps\OpcHub\config\Opc.Hub.jsonnet                   (imports ../../AppServer/config/App.Server.jsonnet, ../../OpcGateway/config/Opc.Gateway.jsonnet)
    apps\OpcHub\config\args\install\args.libsonnet       sqlite; the driver/proc modules by $(ExeDir), the data by $(ProgramData)
    apps\AppServer\config\App.Server.jsonnet
    apps\OpcGateway\config\Opc.Gateway.jsonnet
    apps\OpcGateway\config\introspection\*.jsonnet
    apps\OpcServer\config\Opc.Server.jsonnet + Opc.Server.Install.jsonnet (the overlay the service loads)
    apps\OpcServer\config\args\install\args.libsonnet
    apps\OpcServer\config\pubsub\pumps.libsonnet
    libs\db\config\paths-common.libsonnet
  OpcHub\                                                the product dir (Process::ProductName): created here by the service -> OpcHub.db, ssl\, *.log
    access-meta.jsonnet access-ql.jsonnet app-meta.jsonnet opcGateway-meta.jsonnet common-meta.libsonnet
    sql\  access.mutation (libs/access/config/release.mutation) app.mutation, the sqlite *_ql.sql views
  OpcServer\                                             OpcServer.db, ssl\, *.log
    access-meta.jsonnet access-ql.jsonnet common-meta.libsonnet opcServer-meta.jsonnet
    nodesets\ Opc.Ua.Di.NodeSet2.xml Opc.Ua.IA.NodeSet2.xml Opc.Ua.IA.NodeSet2.examples.xml pumps.NodeSet2.xml
```

The settings are edited in place under `config\`; the meta/sql files are what `args/install` points at.  The service
command lines (composed by the exe's `-install`, `libs/fwk/src/process/process.cpp` - `sc qc Jde.OpcHub` shows them):

```
"C:\Program Files\Jde-Cpp\OpcHub\Jde.Opc.Hub.exe" -settings=C:\ProgramData\Jde-Cpp\config\apps\OpcHub\config\Opc.Hub.jsonnet -include=args/install -sync
"C:\Program Files\Jde-Cpp\OpcServer\Jde.Opc.Server.exe" -settings=C:\ProgramData\Jde-Cpp\config\apps\OpcServer\config\Opc.Server.Install.jsonnet -include=args/install -sync
```

The current-user shortcuts are the same lines with `-c` in front.  `-sync` creates the tables in the fresh `.db` on the
first start and is idempotent afterwards (create-missing tables, recreate the views, upsert the mutations); drop it later
with `sc config Jde.OpcHub binPath= "…"` or by editing the shortcut.

## Uninstall

Add/Remove Programs (or the Start Menu shortcut in a current-user install) stops and deregisters the services (or ends the
console windows and removes the shortcuts/Run entries), removes the program dir, the `config\` mirror and the meta/sql/
nodesets the installer put in the product dirs.  Left in place, deliberately: `OpcHub.db`, `OpcServer.db`, `ssl\`
(certificates and keys - the OPC servers trust them) and the logs.  Delete `C:\ProgramData\Jde-Cpp` by hand for a clean slate.

## Notes

- Reinstalling over an existing install is fine: the services are deregistered and re-registered, the `.db` is kept, the
  installer-owned `sql\` and `nodesets\` are recreated (the settings under `config\` are overwritten - keep a copy of edits).
- `apps/OpcGateway/config/access-opcGateway.mutation` (the gateway's group/role) is not seeded: `createGroup`/`createRole` run
  through the access server's QL, which is up only after the schema sync, so it is a post-start step, not a `dataPaths` seed.
- A split `Jde.AppServer` + `Jde.OpcGateway` pair (`apps/AppServer`, `apps/OpcGateway` - not shipped by this installer) shares
  port 1967 with the hub; the installer stops them and says so.  Deregister them with each exe's `-uninstall`.
- `JDE_PASSCODE` (the private keys' passphrase, `$(JDE_PASSCODE)` in the configs) is unset for a service under LocalSystem, so
  the keys are written in the clear - the documented behaviour of an empty passcode.  Set it as a system environment variable
  before the first start to change that.
- `release.mutation` seeds the access schema without the Google provider rows `access.mutation` (the dev seed) carries; the
  Web UI's Google login needs those added.
- SQL Server instead of sqlite, by hand: `apps/OpcHub/config/args/install-sqlServer/args.libsonnet` is the equivalent profile.
  Copy it to `config\apps\OpcHub\config\args\install-sqlServer\`, put `Jde.DB.Odbc.dll` (from the build's `bin\`) beside the
  exe, create a 64-bit System DSN `jde` ("ODBC Driver 17 for SQL Server", `Trusted_Connection=Yes`) with a database `jde` in
  which `NT AUTHORITY\System` is `db_owner`, copy the `sql\sqlServer\*.sql` scripts of `libs/access`, `apps/AppServer` and
  `apps/OpcGateway` into the product's `sql\`, and re-register the service with `-include=args/install-sqlServer`.
