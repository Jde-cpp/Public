#include <jde/iot/uatypes/Node.h>

#define var const auto
namespace Jde::Iot{
	NodeId::NodeId ( const NodeId& x )ι:
		UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{
		nodeId = x.Copy();
		if( x.namespaceUri.length )
			namespaceUri = UA_String_fromChars( string{ToSV(x.namespaceUri)}.c_str() );
		serverIndex = x.serverIndex;
	}

	NodeId::NodeId( const flat_map<string,string>& x )ι:
		UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{
		try{
			if( auto p = x.find("nsu"); p!=x.end() )
				namespaceUri = UA_String_fromChars( p->second.c_str() );
			if( auto p = x.find("serverindex"); p!=x.end() )
				serverIndex = stoul( p->second );
			if( auto p = x.find("ns"); p!=x.end() )
				nodeId.namespaceIndex = Str::TryTo<UA_UInt16>( p->second ).value_or( 0 );
			if( auto p = x.find("s"); p!=x.end() ){
				nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
				nodeId.identifier.string = UA_String_fromChars( p->second.c_str() );
			}
			else if( auto p = x.find("i"); p!=x.end() ){
				nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
				nodeId.identifier.numeric = stoul( p->second );
			}
			else if( auto p = x.find("b"); p!=x.end() ){
				nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
				auto t = ToUV( p->second );
				UA_ByteString_fromBase64( &nodeId.identifier.byteString, &t );
			}
			else if( auto p = x.find("g"); p!=x.end() ){
				nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
				ToGuid( p->second, nodeId.identifier.guid );
			}
			else
				Debug( ELogTags::App, "No identifier in nodeId" );
		}
		catch( json::exception& e ){
			Critical( ELogTags::App, "Could not create json: {}", e.what() );
		}
	}

	NodeId::NodeId( const json& j )ε:
		UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{
		if( j.find("nsu")!=j.end() )
			namespaceUri = UA_String_fromChars( j["nsu"].get<string>().c_str() );
		if( j.find("serverindex")!=j.end() )
			j.at("serverindex").get_to( serverIndex );
		if( j.find("ns")!=j.end() )
			j.at("ns").get_to( nodeId.namespaceIndex );
		if( j.find("s")!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			nodeId.identifier.string = UA_String_fromChars( j["s"].get<string>().c_str() );
		}
		else if( j.find("i")!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			j.at("i").get_to( nodeId.identifier.numeric );
		}
		else if( j.find("b")!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			var v = ToUV( j["b"].get<string>() );
			UA_ByteString_fromBase64( &nodeId.identifier.byteString, &v );
		}
		else if( j.find("g")!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			ToGuid( j["g"].get<string>(), nodeId.identifier.guid );
		}
	}

	NodeId::NodeId( NodeId&& x )ι:
		UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{
		nodeId = x.Move();
		namespaceUri = x.namespaceUri;
		serverIndex = x.serverIndex;
		UA_ExpandedNodeId_init( &x );
	}
	NodeId::NodeId( Proto::ExpandedNodeId&& x )ι{
		const auto& proto = x.node();
		nodeId.namespaceIndex = proto.namespace_index();
		if( proto.has_numeric() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			nodeId.identifier.numeric = proto.numeric();
		}
		else if( proto.has_string() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			nodeId.identifier.string = UA_String_fromChars( proto.string().c_str() );
		}
		else if( proto.has_byte_string() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			UA_ByteString_allocBuffer( &nodeId.identifier.byteString, proto.byte_string().size() );
			memcpy( nodeId.identifier.byteString.data, proto.byte_string().data(), proto.byte_string().size() );
		}
		else if( proto.has_guid() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			memcpy( &nodeId.identifier.guid, proto.guid().data(), std::min(sizeof(UA_Guid),proto.guid().size()) );
		}
		namespaceUri = UA_String_fromChars( x.namespace_uri().c_str() );
		serverIndex = x.server_index();
	}
	α NodeId::operator=( NodeId&& x )ι->NodeId&{
		nodeId = x.Move();
		namespaceUri=x.namespaceUri;
		serverIndex=x.serverIndex;
		UA_ExpandedNodeId_init( &x );
		return *this;
	}

	α NodeId::ToNodes( google::protobuf::RepeatedPtrField<Proto::ExpandedNodeId>&& proto )ι->flat_set<NodeId>{
		flat_set<NodeId> nodes;
		for( auto& node : proto )
			nodes.emplace( move(node) );
		return nodes;
	}
	α NodeId::Clear()ι->void{
		if( namespaceUri.length )
			UA_String_clear( &namespaceUri );
		if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING && nodeId.identifier.string.length )
			UA_String_clear( &nodeId.identifier.string );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING  && nodeId.identifier.byteString.length )
			UA_ByteString_clear( &nodeId.identifier.byteString );
	}

	α NodeId::operator=( const NodeId& x )ι->NodeId&{
		Clear();
		nodeId = x.Copy();
		if( x.namespaceUri.length )
			namespaceUri = UA_String_fromChars( string{ToSV(x.namespaceUri)}.c_str() );
		serverIndex = x.serverIndex;
		return *this;
	}
	α NodeId::operator<( const NodeId& x )Ι->bool{
		return
			ToSV(namespaceUri)==ToSV(x.namespaceUri) ?
				serverIndex==x.serverIndex ?
					nodeId.namespaceIndex==x.nodeId.namespaceIndex ?
						nodeId.identifierType==x.nodeId.identifierType ?
							nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC ? nodeId.identifier.numeric<x.nodeId.identifier.numeric :
								nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING ? ToSV(nodeId.identifier.string)<ToSV(x.nodeId.identifier.string) :
								nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING ? ToSV(nodeId.identifier.byteString)<ToSV(x.nodeId.identifier.byteString)
							: memcmp( &nodeId.identifier.guid, &x.nodeId.identifier.guid, sizeof(UA_Guid) )<0
						: nodeId.identifierType<x.nodeId.identifierType
					: nodeId.namespaceIndex<x.nodeId.namespaceIndex
				: serverIndex<x.serverIndex
			: ToSV(namespaceUri)<ToSV(x.namespaceUri);
	}

	α NodeId::Copy()Ι->UA_NodeId{
		UA_NodeId y{};
    y.namespaceIndex = nodeId.namespaceIndex;
    y.identifierType = nodeId.identifierType;
		if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			y.identifier.numeric = nodeId.identifier.numeric;
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING )
			y.identifier.string = UA_String_fromChars( string{ToSV(nodeId.identifier.string)}.c_str() );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			y.identifier.byteString =  UA_BYTESTRING_ALLOC( string{ToSV(nodeId.identifier.byteString)}.c_str() );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			y.identifier.guid = nodeId.identifier.guid;
		return y;
	}

	α NodeId::Move()ι->UA_NodeId{
		UA_NodeId y{};
    y.namespaceIndex = nodeId.namespaceIndex;
    y.identifierType = nodeId.identifierType;
		if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			y.identifier.numeric = nodeId.identifier.numeric;
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING )
	    y.identifier.string = nodeId.identifier.string;
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			y.identifier.byteString = nodeId.identifier.byteString;
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			y.identifier.guid = nodeId.identifier.guid;
		memset( &nodeId, 0, sizeof(UA_NodeId) );
		return y;
	}

	α to_json( nlohmann::json& n, const UA_NodeId& nodeId )ι->void
	{
		n["ns"] = nodeId.namespaceIndex;
		const UA_NodeIdType type = nodeId.identifierType;
		if( type==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			n["i"]=nodeId.identifier.numeric;
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_STRING )
			n["s"]=ToSV( nodeId.identifier.string );
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			n["g"] = ToJson( nodeId.identifier.guid );
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			n["b"] = ByteStringToJson( nodeId.identifier.byteString );
	}
	α ToJson( const UA_NodeId& nodeId )ι->json{
		json j;
		to_json( j, nodeId );
		return j;
	}

	α ToJson( const UA_ExpandedNodeId& x )ι->json{
		json j;
		if( x.namespaceUri.length )
			j["nsu"] = ToSV(x.namespaceUri);
		if( x.serverIndex )
			j["serverindex"] = x.serverIndex;
		to_json( j, x.nodeId );
		return j;
	}
	α NodeId::ToJson()Ι->nlohmann::json{
		return Iot::ToJson( *this );
	}

	α NodeId::to_string()Ι->string{
		return ToJson().dump();
	}
	α NodeId::ToProto()Ι->Proto::ExpandedNodeId{
		Proto::ExpandedNodeId y;
		if( namespaceUri.length )
			y.set_allocated_namespace_uri( new string{ToSV(namespaceUri)} );
		y.set_server_index( serverIndex );
		y.set_allocated_node( new Proto::NodeId{ToNodeProto()} );
		return y;
	}
	α NodeId::ToNodeProto()Ι->Proto::NodeId{
		Proto::NodeId y;
		y.set_namespace_index( nodeId.namespaceIndex );
		if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			y.set_numeric( nodeId.identifier.numeric );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING )
			y.set_allocated_string( new string{ToSV(nodeId.identifier.string)} );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			y.set_allocated_byte_string( new string{ToSV(nodeId.identifier.byteString)} );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			y.set_guid( ToBinaryString(nodeId.identifier.guid) );
		return y;
	}

	std::size_t NodeIdHash::operator()(const NodeId& n)Ι{
		std::size_t seed = 0;
		boost::hash_combine( seed, ToSV(n.namespaceUri) );
		boost::hash_combine( seed, n.serverIndex );
		var& nodeId = n.nodeId;
		if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			boost::hash_combine( seed, nodeId.identifier.numeric );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING )
			boost::hash_combine( seed, ToSV(nodeId.identifier.string) );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			boost::hash_combine( seed, ToJson(nodeId.identifier.guid).dump() );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			boost::hash_combine( seed, ToSV(nodeId.identifier.byteString) );
		return seed;//4452845294327023648
	}
}