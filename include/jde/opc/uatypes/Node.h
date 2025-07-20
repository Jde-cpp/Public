#pragma once
#ifndef NODE_H
#define NODE_H
#include "../exports.h"
#include "helpers.h"

namespace Jde::DB{ struct Row; struct Value; }
namespace Jde::Opc{
	struct ΓOPC ExNodeId : UA_ExpandedNodeId{
		ExNodeId()ι:UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{}
		ExNodeId( UA_NodeId&& x )ι:UA_ExpandedNodeId{move(x), UA_EXPANDEDNODEID_NULL.namespaceUri, UA_EXPANDEDNODEID_NULL.serverIndex}{}
		ExNodeId( const UA_NodeId& x )ι:ExNodeId{}{ UA_NodeId_copy( &x, &nodeId ); }
		ExNodeId( const UA_ExpandedNodeId& x )ι:ExNodeId{}{ UA_ExpandedNodeId_copy( &x, this ); }
		ExNodeId( const flat_map<string,string>& x )ε;//rest params
		explicit ExNodeId( const jvalue& j )ε;
		ExNodeId( UA_UInt32 numeric )ι:ExNodeId{UA_NodeId{0, UA_NODEIDTYPE_NUMERIC, {numeric}}}{}
		ExNodeId( const ExNodeId& x )ι;
		//NodeId( Proto::ExpandedNodeId&& x )ι;
		ExNodeId( ExNodeId&& x )ι;
		ExNodeId( DB::Row& r, uint8 nsIndex, bool extended=false )ε;
		α operator=( ExNodeId&& x )ι->ExNodeId&;
		~ExNodeId(){ Clear(); }
		α operator<( const ExNodeId& x )Ι->bool;
		α operator=( const ExNodeId& x )ι->ExNodeId&;
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
		Ω ToJson( flat_set<ExNodeId> nodes )ι->jarray{ jarray j; for_each(nodes, [&j](const auto& n){ j.push_back( n.ToJson() ); }); return j; }
		α to_string()Ι->string;
	};
	Ξ operator==( const ExNodeId& x, const ExNodeId& y )ι->bool{ return !(x<y) && !(y<x); }
	α ToJson( const UA_NodeId& nodeId )ι->jobject;
	α ToJson( const UA_ExpandedNodeId& nodeId )ι->jobject;

	struct NodeIdHash{
		uint operator()(const ExNodeId& n)Ι;
	};
}
#endif