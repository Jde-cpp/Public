// Opc.Server.Emulator.jsonnet against a Jde.Opc.Hub instead of a split AppServer - i.e. Opc.Server.Hub.jsonnet's login
// TLS anchor plus the emulator's Part 14 DataSetReader.  Pair it with emulator/config/Opc.PlcEmulator.Hub.jsonnet.
// See Opc.Server.Emulator.jsonnet for why the reader is an overlay and not the default.
local base = import 'Opc.Server.Hub.jsonnet';
function( sync=false ) base( sync ) + {
	opcServer+:{
		pubsub: import 'pubsub/pumps.libsonnet'
	}
}
