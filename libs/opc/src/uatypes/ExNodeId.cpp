#include <jde/opc/uatypes/ExNodeId.h>
#include <jde/db/Row.h>
#include <jde/db/Value.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/UAException.h>

#define let const auto
namespace Jde::Opc{
	ExNodeId::ExNodeId ( const ExNodeId& x )ι:
		UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{
		nodeId = x.Copy();
		if( x.namespaceUri.length )
			namespaceUri = UA_String_fromChars( string{ToSV(x.namespaceUri)}.c_str() );
		serverIndex = x.serverIndex;
	}

	ExNodeId::ExNodeId( const flat_map<string,string>& x )ε:
		UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{
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
			DBGT( ELogTags::App, "No identifier in nodeId" );
	}

	ExNodeId::ExNodeId( const jvalue& j )ε:
		UA_ExpandedNodeId{
			NodeId::FromJson(j),
			UA_String_fromChars(string{Json::FindDefaultSV(j, "nsu")}.c_str()),
			Json::FindNumber<UA_UInt32>(j, "serverindex").value_or(0)
		}
	{}

	ExNodeId::ExNodeId( ExNodeId&& x )ι:
		UA_ExpandedNodeId{UA_EXPANDEDNODEID_NULL}{
		nodeId = x.Move();
		namespaceUri = x.namespaceUri;
		serverIndex = x.serverIndex;
		UA_ExpandedNodeId_init( &x );
	}

	ExNodeId::ExNodeId( DB::Row& r, uint8 index, bool extended )ε{
		nodeId.namespaceIndex = r.Get<uint16>( index );
		if( !r.IsNull(index+1) ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			nodeId.identifier.numeric = r.Get<UA_UInt32>( index+1 );
		}
		else if( !r.IsNull(index+2) ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			nodeId.identifier.string = UA_String_fromChars( r.GetString(index+2).c_str() );
		}
		else if( !r.IsNull(index+3) ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			let guid = r.GetGuid( index+3 );
			::memcpy( &nodeId.identifier.guid, &guid, sizeof(UA_Guid) );
		}
		else if( !r.IsNull(index+4) ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			auto bytes = r.GetBytes( index+4 );
			UA_ByteString_allocBuffer( &nodeId.identifier.byteString, bytes.size() );
			::memcpy( nodeId.identifier.byteString.data, bytes.data(), bytes.size() );
		}
		namespaceUri = extended ? UA_String_fromChars( r.GetString(index+5).c_str() ) : UA_STRING_NULL;
		serverIndex = extended ? r.GetUInt32Opt( index+6 ).value_or( 0 ) : 0;
	}

	α ExNodeId::InsertParams( bool extended )Ι->vector<DB::Value>{
		vector<DB::Value> params; params.reserve( 64 );
		using enum UA_NodeIdType;
		params.emplace_back( nodeId.namespaceIndex );
		params.emplace_back( IsNumeric() ? DB::Value{*Numeric()} : DB::Value{} );
		params.emplace_back( IsString() ? DB::Value{*String()} : DB::Value{} );
		params.emplace_back( IsGuid() ? DB::Value{*Guid()} : DB::Value{vector<uint8_t>{}} );
		params.emplace_back( IsBytes() ? DB::Value{FromByteString(*Bytes())} : DB::Value{vector<uint8_t>{}} );
		if( extended ){
			params.emplace_back( namespaceUri.length ? DB::Value{ToString(namespaceUri)} : DB::Value{} );
			params.emplace_back( serverIndex );
		}
		return params;
	}

	α ExNodeId::SetNodeId( UA_NodeId&& x )ι->void{
		nodeId.namespaceIndex = x.namespaceIndex;
		nodeId.identifierType = x.identifierType;
		nodeId.identifier = x.identifier;
    UA_String_clear( &namespaceUri );
    serverIndex = 0;
		UA_NodeId_clear(&x);
	}

	α ExNodeId::operator=( ExNodeId&& x )ι->ExNodeId&{
		nodeId = x.Move();
		namespaceUri=x.namespaceUri;
		serverIndex=x.serverIndex;
		UA_ExpandedNodeId_init( &x );
		return *this;
	}

	α ExNodeId::Clear()ι->void{
		if( namespaceUri.length )
			UA_String_clear( &namespaceUri );
		if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING && nodeId.identifier.string.length )
			UA_String_clear( &nodeId.identifier.string );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING  && nodeId.identifier.byteString.length )
			UA_ByteString_clear( &nodeId.identifier.byteString );
	}

	α ExNodeId::operator=( const ExNodeId& x )ι->ExNodeId&{
		Clear();
		nodeId = x.Copy();
		if( x.namespaceUri.length )
			namespaceUri = UA_String_fromChars( string{ToSV(x.namespaceUri)}.c_str() );
		serverIndex = x.serverIndex;
		return *this;
	}
	α ExNodeId::operator<( const ExNodeId& x )Ι->bool{
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

	α ExNodeId::Copy()Ι->UA_NodeId{
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

	α ExNodeId::Move()ι->UA_NodeId{
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

	α ExNodeId::ToJson()Ι->jobject{
		return Opc::ToJson( *this );
	}

	α ExNodeId::to_string()Ι->string{
		return serialize( ToJson() );
	}
	α NodeIdHash::operator()(const ExNodeId& n)Ι->uint{
		uint seed = 0;
		boost::hash_combine( seed, ToSV(n.namespaceUri) );
		boost::hash_combine( seed, n.serverIndex );
		let& nodeId = n.nodeId;
		if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			boost::hash_combine( seed, nodeId.identifier.numeric );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING )
			boost::hash_combine( seed, ToSV(nodeId.identifier.string) );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			boost::hash_combine( seed, serialize(ToJson(nodeId.identifier.guid)) );
		else if( nodeId.identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			boost::hash_combine( seed, ToSV(nodeId.identifier.byteString) );
		return seed;//4452845294327023648
	}
	Ω toJson( jobject& j, const UA_NodeId& nodeId )ι->jobject{
		if( nodeId.namespaceIndex )
			j["ns"] = nodeId.namespaceIndex;
		const UA_NodeIdType type = nodeId.identifierType;
		if( type==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			j["i"] = nodeId.identifier.numeric;
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_STRING )
			j["s"] = ToSV( nodeId.identifier.string );
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			j["g"] = ToJson( nodeId.identifier.guid );
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			j["b"] = ByteStringToJson( nodeId.identifier.byteString );
		return j;
	}
}
namespace Jde{
	α Opc::ToJson( const UA_ExpandedNodeId& x )ε->jobject{
		jobject j;
		if( x.namespaceUri.length )
			j["nsu"] = ToSV(x.namespaceUri);
		if( x.serverIndex )
			j["serverindex"] = x.serverIndex;
		toJson( j, x.nodeId );
		return j;
	}
}
