#pragma once
#include <jde/opc/uatypes/ExNodeId.h>

namespace Jde::Opc::ProtoUtils{
	α ToNodeId( const Proto::NodeId& proto )ι->NodeId;
	α ToNodeId( const NodeId& id )ι->Proto::NodeId;
	α ToExNodeId( const Proto::ExpandedNodeId& proto )ι->ExNodeId;
	α ToNodeIds( google::protobuf::RepeatedPtrField<Proto::NodeId>&& proto )ι->flat_set<NodeId>;
}