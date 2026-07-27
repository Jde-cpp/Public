// Soak-client config. No database - the client only speaks to the AppServer (:1967) and the gateway's WebSocket (:1968).
// Durations/paths under `soak` are defaults; soak.sh overrides per-run via CLI flags (-duration=… beats the setting).
// `soak.servers` lists the gateway connections to drive; an entry with `flag` only runs when that CLI arg is present
// (-external here - soak.sh --external), so Linux/CI runs are unchanged. For the flagged entry, -externalUrl=/
// -externalUri=/-externalUser=/-externalPwd= override url/certificateUri/user/password (never commit a real password).
local logsDir = std.extVar("logsDir");
{
	server:{
		host: "localhost",
		port: 1967,
		isSsl: false
	},
	credentials:{
		name: "OpcSoak"
	},
	soak:{
		gatewayHost: "localhost",
		gatewayPort: 1968,
		gatewayProduct: "OpcGateway", //where -createCert writes: $(companyDir)/<gatewayProduct>/ssl - must match the gateway's ProductName.
		duration: "PT24H",
		writePeriod: "PT1S",
		pushTimeout: "PT5S",
		statusPeriod: "PT1M",
		quietInterval: "PT6H",
		quietPeriod: "PT10M",
		servers: [
			{
				target: "OpcSoak", name: "Soak test server", description: "Soak test connection",
				certificateUri: "urn:open62541.server.application",
				url: "opc.tcp://127.0.0.1:4840",
				nodes: [ { ns: 4, id: 6017 } ] //writable numeric var from IA examples nodeset - same node SubscribeTests uses.
			},
			{
				flag: "-external", //leg active only when this CLI arg is present.
				target: "ExternalSoak", name: "External soak server",
				description: "Externally-managed OPC-UA server (not launched/monitored by soak.sh)",
				certificateUri: "", //set via -externalUri= to the server's application URI (raw; %20-encoded at use) - required for Basic256Sha256; empty falls back to SecurityPolicy None and no client cert.
				url: "opc.tcp://127.0.0.1:49320",
				user: "soak", //server account with tag-write access - presence of `user` makes the leg log in.
				password: "", //set via -externalPwd=.
				nodes: [ { ns: 2, s: "Data Type Examples.16 Bit Device.K Registers.Long1" } ] //writable static Int32 tag - an Int16 tag overflows the PT1S counter at ~9.1h.
			}
		]
	},
	http:{
		ssl:{
			productName: "Opc.Soak" //keys under $(companyDir)/Opc.Soak/ssl on every OS - windows has no .rc ProductName for this exe.
		}
	},
	logging:{
		spd:{
			tags:{
				default: "Information",
				uaNet: "Warning",
				uaClient: "Warning",
				uaSecure: "Warning",
				uaSession: "Warning"
			},
			sinks:{
				console:{},
				file:{ path: logsDir, md: false }
			}
		}
	},
	workers:{
		drive:{ threads: 1 },
		executor:{ threads: 2 }
	}
}
