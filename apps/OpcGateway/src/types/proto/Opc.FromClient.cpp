#include "Opc.FromClient.h"


namespace Jde::Opc::Gateway{
	α FromClientUtils::ToNode( Proto::ExpandedNodeId&& x )ι->ExNodeId{
		ExNodeId y;
		const auto& proto = x.node();
		y.nodeId.namespaceIndex = (int16)proto.namespace_index();
		if( proto.has_numeric() ){
			y.nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			y.nodeId.identifier.numeric = proto.numeric();
		}
		else if( proto.has_string() ){
			y.nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			y.nodeId.identifier.string = UA_String_fromChars( proto.string().c_str() );
		}
		else if( proto.has_byte_string() ){
			y.nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			UA_ByteString_allocBuffer( &y.nodeId.identifier.byteString, proto.byte_string().size() );
			memcpy( y.nodeId.identifier.byteString.data, proto.byte_string().data(), proto.byte_string().size() );
		}
		else if( proto.has_guid() ){
			y.nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			memcpy( &y.nodeId.identifier.guid, proto.guid().data(), std::min(sizeof(UA_Guid),proto.guid().size()) );
		}
		y.namespaceUri = UA_String_fromChars( x.namespace_uri().c_str() );
		y.serverIndex = x.server_index();
		return y;
	}
	α FromClientUtils::ToNodes( google::protobuf::RepeatedPtrField<Proto::ExpandedNodeId>&& proto )ι->flat_set<ExNodeId>{
		flat_set<ExNodeId> nodes;
		for( auto& p : proto )
			nodes.insert( ToNode(move(p)) );
		return nodes;
	}
}