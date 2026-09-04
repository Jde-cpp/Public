# Jde.Opc.Hub

The AppServer and the OpcGateway in one process: `Jde.App.ServerLib` + `Jde.Opc.GatewayLib` linked into one exe
(`src/hubStartup.cpp` composes the two startups), with the gateway's app client answered in-process
(`src/HubAppClient.h`) instead of over the loopback login + websocket a split gateway uses.  The standalone
`Jde.App.Server` and `Jde.Opc.Gateway` keep building for split (N gateways per AppServer) deployments.

| | value |
|---|---|
| exe / lib / tests | `Jde.Opc.Hub` / `Jde.Opc.HubLib` / `Jde.Opc.Hub.Tests` |
| `Process::AppName()` (service name, `connections{programName}`) | `Jde.OpcHub` |
| `Process::ProductName()` (`$(ProgramData)/Jde-Cpp/<product>`: certs, issued OPC certs, app data) | `OpcHub` |
| settings / log | `config/Opc.Hub.jsonnet` / `Opc.Hub.log` (derived by `Settings::FileStem()`) |
| port | one, 1967 (`/http`): the AppServer's REST + app-protocol socket at `/`, the gateway's REST + OPC socket at `/opc`, one `/graphql` over access+app+gateway |

## Run

```bash
D=$JDE_DIR/.claude/skills/run-services/driver.sh
$D start hub                 # never beside the split appserver - they share 1967
$D start opcserver-hub       # the OpcServer with config/Opc.Server.Hub.jsonnet: anchors the hub's cert for its login
$D smoke hub
curl -s localhost:1967/opcGateways   # {"servers":[{"host":"localhost","port":1967,"instanceName":"OpcHub.debug"}]}
curl -s localhost:1967/ErrorCodes?scs=2150891520 && curl -s localhost:1967/GoogleAuthClientId   # both apps' routes, one port
```

Foreground: `Jde.Opc.Hub -c -tests -settings=$JDE_DIR/apps/OpcHub/config/Opc.Hub.jsonnet -include=args/mysql` from
`<buildDir>/runtime` (`-include=args/sqlite -arg path=<file>` for sqlite).

## Config

`config/Opc.Hub.jsonnet` imports both production configs and picks every top-level key explicitly (jsonnet `+`
replaces whole sub-objects).  `config/args/<dialect>/args.libsonnet` mounts `access` + `app` + `gateway` in one
catalog with the split apps' table prefixes, so the hub runs against the data a split AppServer + gateway created.
No meta/sql of its own - the mounts point at `apps/AppServer/config` and `apps/OpcGateway/config`.

Certs: `ProductName` puts the hub's tree under `$(ProgramData)/Jde-Cpp/OpcHub`.  The OpcServer and the PLC emulator
anchor the split AppServer's cert for their login TLS, so against a hub they need the overlays
`apps/OpcServer/config/Opc.Server.Hub.jsonnet` / `apps/OpcServer/emulator/config/Opc.PlcEmulator.Hub.jsonnet`
(`caFile` = the hub's `OpcHub.pem`); the OpcServer's args already trust `certsDir("OpcHub")` for the gateway role's
OPC client certs.

## Tests

`Jde.Opc.Hub.Tests` (`tests/`) embeds the hub (1973) plus an OpcServer (1975, opc.tcp 4842) - ports nothing else binds,
so it runs beside a live hub.  `ctest --timeout 300 -R Jde.Opc.Hub.Tests`.

## Frontend

Unchanged endpoint (`applicationServer` 1967).  The SPA discovers the gateway role through `/opcGateways` (same host:port)
and opens its OPC websocket on `/opc` (`Gateway.socketPath` in `web/opc/control/.../gateway-service.ts`); a standalone
gateway ignores the path, so one build serves both deployments.  The hub's one `connections{}` row (`Jde.OpcHub`) routes to
the gateway page (`app-resolver.ts`).

## Not done here

Soak support (`apps/OpcGateway/soak/soak.sh` is shaped around three exes), the Windows installer
(`apps/OpcGateway/setup` is still on the split layout), source consolidation under this directory.
