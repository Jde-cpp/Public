// Opc.PlcEmulator.jsonnet against a Jde.Opc.Hub: the login TLS anchor is the hub's http cert (Opc.Hub.jsonnet's /http/app
// commonName "OpcHub") instead of the split AppServer's.  Nothing else changes - the hub's AppServer role listens on 1967.
(import 'Opc.PlcEmulator.jsonnet') + {
	web+:{ client+:{ ssl+:{ caFile: "$(ProgramData)/Jde-Cpp/OpcHub/ssl/certs/OpcHub.pem" } } }
}
