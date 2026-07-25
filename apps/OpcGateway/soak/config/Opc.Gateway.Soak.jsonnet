// Soak overlay: production gateway config with sync=true baked in and log tags flattened - the production map has
// http/socket read+write at Debug and app/browse/ql at Trace, unbounded over 24h (the text sink never rotates).
// gateway.pingInterval/ttl stay at production values - the idle-teardown path is under test.
// Launch with -include=../../config/args/sqlite (relative to this file) and -arg path=<run>/db/gateway.db.
local base = (import '../../config/Opc.Gateway.jsonnet')(sync=true);
base + {
	ql+: {
		//introspection paths resolve relative to the settings file's directory - point back at the production config dir.
		introspection: [ '../../config/' + p for p in base.ql.introspection ]
	},
	logging+: {
		spd+: {
			tags: {
				default: "Information",
				monitoring: "Information",
				processingLoop: "Information",
				uaNet: "Warning",
				uaClient: "Warning",
				uaSecure: "Warning",
				uaSession: "Warning",
				uaServer: "Warning",
				uaUser: "Warning",
				uaSecurity: "Warning",
				uaPubSub: "Warning",
				uaDiscovery: "Warning"
			}
		}
	}
}
