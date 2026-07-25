// Soak-client config. No database - the client only speaks to the AppServer (:1967) and the gateway's WebSocket (:1968).
// Durations/paths under `soak` are defaults; soak.sh overrides per-run via CLI flags (-duration=… beats the setting).
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
	opc:{
		urn: "urn:open62541.server.application",
		url: "opc.tcp://127.0.0.1:4840"
	},
	soak:{
		target: "OpcSoak",
		gatewayHost: "localhost",
		gatewayPort: 1968,
		gatewayProduct: "OpcGateway", //where -createCert writes: $(companyDir)/<gatewayProduct>/ssl - must match the gateway's ProductName.
		duration: "PT24H",
		writePeriod: "PT1S",
		pushTimeout: "PT5S",
		statusPeriod: "PT1M",
		quietInterval: "PT6H",
		quietPeriod: "PT10M",
		nodes: [ { ns: 4, id: 6017 } ] //writable numeric var from IA examples nodeset - same node SubscribeTests uses.
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
