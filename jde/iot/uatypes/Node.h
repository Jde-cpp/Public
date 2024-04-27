#pragma once
#ifndef NODE_H
#define NODE_H
#include "../Exports.h"
#include "helpers.h"

namespace Jde::Iot{
	struct ΓI NodeId : UA_ExpandedNodeId{
		NodeId()ι:UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{}
		NodeId( UA_NodeId&& x )ι:UA_ExpandedNodeId{move(x), UA_EXPANDEDNODEID_NULL.namespaceUri, UA_EXPANDEDNODEID_NULL.serverIndex}{}
		NodeId( const UA_NodeId& x )ι:NodeId{}{ UA_NodeId_copy( &x, &nodeId ); }
		NodeId( const UA_ExpandedNodeId& x )ι:NodeId{}{ UA_ExpandedNodeId_copy( &x, this ); }
		NodeId( const flat_map<string,string>& x )ι;
		NodeId( const json& j )ε;
		NodeId( const NodeId& x )ι;
		NodeId( Proto::ExpandedNodeId&& x )ι;
		NodeId( NodeId&& x )ι;
		Ω ToNodes( google::protobuf::RepeatedPtrField<Proto::ExpandedNodeId>&& proto )ι->flat_set<NodeId>;
		α operator=( NodeId&& x )ι->NodeId&;
		~NodeId(){ Clear(); }
		α operator<( const NodeId& x )Ι->bool;
		α operator=( const NodeId& x )ι->NodeId&;
		α Clear()ι->void;
		α Copy()Ι->UA_NodeId;
		α Move()ι->UA_NodeId;
		α ToJson()Ι->json;
		Ω ToJson( flat_set<NodeId> nodes )ι->json{ auto j=json::array(); for_each(nodes, [&j](const auto& n){ j.push_back( n.ToJson() ); }); return j; }
		α ToProto()Ι->Proto::ExpandedNodeId;
		α ToNodeProto()Ι->Proto::NodeId;
		α to_string()Ι->string;
	};
	Ξ operator==( const NodeId& x, const NodeId& y )ι->bool{ return !(x<y) && !(y<x); }
	α ToJson( const UA_NodeId& nodeId )ι->json;
	α ToJson( const UA_ExpandedNodeId& nodeId )ι->json;

	struct NodeIdHash{
		uint operator()(const NodeId& n)Ι;
	};
}
#endif