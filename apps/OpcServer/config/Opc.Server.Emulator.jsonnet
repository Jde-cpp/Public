// PLC-emulator overlay: the production OpcServer config plus the Part 14 DataSetReader the emulator publishes into.
// Run the OpcServer with this whenever the emulator's default transport (-transport=pubsub) is in play; the stock
// Opc.Server.jsonnet deliberately ships without a reader (-transport=write needs no overlay - that path is a normal
// authenticated client session).
//
// Why it is an overlay and not the default: a DataSetReader lands received fields in its target variables through the
// server-internal write - no session, no OpcAuthorize (include/jde/opc/pubsub/PubSub.h) - and its only filter is the
// publisherId/writerGroupId/dataSetWriterId triple, three constants sitting in a tracked config file.  Anyone who can
// reach the url therefore drives pump*.motorRpm with no identity at all, and the multicast default (opc.udp://224.0.0.22)
// means "the multicast domain".  This build has no SKS, so the spec's answer to that is unavailable; the containment is
// that only a deployment that asks for the emulator gets the reader, and PubSub::Reader WARNs at startup when it does.
//
// Launch as Opc.Server.jsonnet, e.g. -include=config/args/sqlite, with -config pointing here.
local base = import 'Opc.Server.jsonnet';
function( sync=false ) base( sync ) + {
	opcServer+:{
		//One contract for both ends - the emulator's /emulator/pubsub imports the same file.
		pubsub: import 'pubsub/pumps.libsonnet'
	}
}
