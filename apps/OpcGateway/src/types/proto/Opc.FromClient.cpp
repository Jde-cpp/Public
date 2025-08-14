#include "Opc.FromClient.h"


namespace Jde::Opc::Gateway{
	α FromClientUtils::ToNode( Proto::NodeId&& proto )ι->NodeId{
		NodeId y;
		y.namespaceIndex = (int16)proto.namespace_index();
		if( proto.has_numeric() ){
			y.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			y.identifier.numeric = proto.numeric();
		}
		else if( proto.has_string() ){
			y.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			y.identifier.string = UA_String_fromChars( proto.string().c_str() );
		}
		else if( proto.has_byte_string() ){
			y.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			UA_ByteString_allocBuffer( &y.identifier.byteString, proto.byte_string().size() );
			memcpy( y.identifier.byteString.data, proto.byte_string().data(), proto.byte_string().size() );
		}
		else if( proto.has_guid() ){
			y.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			memcpy( &y.identifier.guid, proto.guid().data(), std::min(sizeof(UA_Guid),proto.guid().size()) );
		}
//		y.namespaceUri = UA_String_fromChars( x.namespace_uri().c_str() );
//		y.serverIndex = x.server_index();
		return y;
	}
	α FromClientUtils::ToNodes( google::protobuf::RepeatedPtrField<Proto::NodeId>&& proto )ι->flat_set<NodeId>{
		flat_set<NodeId> nodes;
		for( auto& p : proto )
			nodes.insert( ToNode(move(p)) );
		return nodes;
	}
}