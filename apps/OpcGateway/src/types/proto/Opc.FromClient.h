#pragma once
namespace Jde::Opc{ struct ExNodeId; struct Value; }

namespace Jde::Opc::Gateway::FromClientUtils{
	α ToNode( Proto::ExpandedNodeId&& proto )ι->ExNodeId;
	α ToNodes( google::protobuf::RepeatedPtrField<Proto::ExpandedNodeId>&& proto )ι->flat_set<ExNodeId>;
}
