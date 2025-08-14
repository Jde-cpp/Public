#pragma once
namespace Jde::Opc{ struct NodeId; struct Value; }

namespace Jde::Opc::Gateway::FromClientUtils{
	α ToNode( Proto::NodeId&& proto )ι->NodeId;
	α ToNodes( google::protobuf::RepeatedPtrField<Proto::NodeId>&& proto )ι->flat_set<NodeId>;
}
