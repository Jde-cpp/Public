#include "opc.Common.h"

namespace Jde::Opc{
	α ProtoUtils::ToExNodeId( const Proto::ExpandedNodeId& proto )ι->ExNodeId{
		ExNodeId y;
		y.namespaceUri = AllocUAString( proto.namespace_uri() );
		y.serverIndex = (uint32)proto.server_index();
		y.nodeId = ToNodeId( proto.node() );
		return y;
	}
	α ProtoUtils::ToNodeId( const NodeId& id )ι->Proto::NodeId{
		Proto::NodeId y;
		y.set_namespace_index( id.namespaceIndex );
		switch( id.identifierType ){
			case UA_NodeIdType::UA_NODEIDTYPE_NUMERIC:
				y.set_numeric( id.identifier.numeric );
				break;
			case UA_NodeIdType::UA_NODEIDTYPE_STRING:
				y.set_string( string((char*)id.identifier.string.data, id.identifier.string.length) );
				break;
			case UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING:
				y.set_byte_string( string((char*)id.identifier.byteString.data, id.identifier.byteString.length) );
				break;
			case UA_NodeIdType::UA_NODEIDTYPE_GUID:
				y.set_guid( string((char*)&id.identifier.guid, sizeof(UA_Guid)) );
				break;
		}
		return y;
	}
	α ProtoUtils::ToNodeId( const Proto::NodeId& id )ι->NodeId{
		NodeId y;
		y.namespaceIndex = ( int16 )id.namespace_index();
		if( id.has_numeric() ){
			y.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			y.identifier.numeric = id.numeric();
		}
		else if( id.has_string() ){
			y.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			y.identifier.string = UA_String_fromChars( id.string().c_str() );
		}
		else if( id.has_byte_string() ){
			y.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			UA_ByteString_allocBuffer( &y.identifier.byteString, id.byte_string().size() );
			memcpy( y.identifier.byteString.data, id.byte_string().data(), id.byte_string().size() );
		}
		else if( id.has_guid() ){
			y.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			memcpy( &y.identifier.guid, id.guid().data(), std::min(sizeof(UA_Guid),id.guid().size()) );
		}
		return y;
	}
	α ProtoUtils::ToNodeIds( google::protobuf::RepeatedPtrField<Proto::NodeId>&& proto )ι->flat_set<NodeId>{
		flat_set<NodeId> nodes;
		for( auto& p : proto )
			nodes.insert( ToNodeId(move(p)) );
		return nodes;
	}
}