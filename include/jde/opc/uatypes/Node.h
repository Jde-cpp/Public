#pragma once
#ifndef NODE_H
#define NODE_H
#include "../exports.h"
#include "helpers.h"
#include <jde/opc/types/proto/Opc.Common.pb.h>

namespace Jde::DB{ struct Row; struct Value; }
namespace Jde::Opc{
	struct ΓOPC NodeId : UA_ExpandedNodeId{
		NodeId()ι:UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{}
		NodeId( UA_NodeId&& x )ι:UA_ExpandedNodeId{move(x), UA_EXPANDEDNODEID_NULL.namespaceUri, UA_EXPANDEDNODEID_NULL.serverIndex}{}
		NodeId( const UA_NodeId& x )ι:NodeId{}{ UA_NodeId_copy( &x, &nodeId ); }
		NodeId( const UA_ExpandedNodeId& x )ι:NodeId{}{ UA_ExpandedNodeId_copy( &x, this ); }
		NodeId( const flat_map<string,string>& x )ε;//rest params
		explicit NodeId( const jvalue& j )ε;
		NodeId( UA_UInt32 numeric )ι:NodeId{UA_NodeId{0, UA_NODEIDTYPE_NUMERIC, {numeric}}}{}
		NodeId( const NodeId& x )ι;
		NodeId( Proto::ExpandedNodeId&& x )ι;
		NodeId( NodeId&& x )ι;
		NodeId( DB::Row& r, uint8 nsIndex, bool extended=false )ε;
		Ω ToNodes( google::protobuf::RepeatedPtrField<Proto::ExpandedNodeId>&& proto )ι->flat_set<NodeId>;
		α operator=( NodeId&& x )ι->NodeId&;
		~NodeId(){ Clear(); }
		α operator<( const NodeId& x )Ι->bool;
		α operator=( const NodeId& x )ι->NodeId&;
		α SetNodeId( UA_NodeId&& x )ι->void;
		β InsertParams( bool extended )Ι->vector<DB::Value>;

		α NsIndex()Ι->UA_UInt16{ return nodeId.namespaceIndex; }
		α Numeric()Ι->optional<UA_UInt32>{ return nodeId.identifierType==UA_NODEIDTYPE_NUMERIC ? nodeId.identifier.numeric : optional<UA_UInt32>{}; }
		α String()Ι->optional<string>{ return nodeId.identifierType==UA_NODEIDTYPE_STRING ? ToString(nodeId.identifier.string) : optional<string>{}; }
		α Guid()Ι->optional<uuid>{ return nodeId.identifierType==UA_NODEIDTYPE_GUID ? ToGuid(nodeId.identifier.guid) : optional<uuid>{}; }
		α Bytes()Ι->optional<UA_ByteString>{ return nodeId.identifierType==UA_NODEIDTYPE_BYTESTRING ? nodeId.identifier.byteString : optional<UA_ByteString>{}; }

		α IsNumeric()Ι{ return nodeId.identifierType==UA_NODEIDTYPE_NUMERIC; }
		α IsString()Ι{ return nodeId.identifierType==UA_NODEIDTYPE_STRING; }
		α IsGuid()Ι{ return nodeId.identifierType==UA_NODEIDTYPE_GUID; }
		α IsBytes()Ι{ return nodeId.identifierType==UA_NODEIDTYPE_BYTESTRING; }
		Ω IsSystem( const UA_NodeId& id )ι->bool;
		α IsSystem()Ι->bool{ return IsNumeric() && IsSystem(nodeId); }

		α Clear()ι->void;
		α Copy()Ι->UA_NodeId;
		α Move()ι->UA_NodeId;
		α ToJson()Ι->jobject;
		Ω ToJson( flat_set<NodeId> nodes )ι->jarray{ jarray j; for_each(nodes, [&j](const auto& n){ j.push_back( n.ToJson() ); }); return j; }
		α ToProto()Ι->Proto::ExpandedNodeId;
		α ToNodeProto()Ι->Proto::NodeId;
		α to_string()Ι->string;
	};
	Ξ operator==( const NodeId& x, const NodeId& y )ι->bool{ return !(x<y) && !(y<x); }
	α ToJson( const UA_NodeId& nodeId )ι->jobject;
	α ToJson( const UA_ExpandedNodeId& nodeId )ι->jobject;

	struct NodeIdHash{
		uint operator()(const NodeId& n)Ι;
	};
}
#endif