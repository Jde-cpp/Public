#include <jde/opc/uatypes/NodeId.h>
#include <jde/db/Row.h>
#include <jde/ql/types/TableQL.h>

#define let const auto
namespace Jde::Opc{
	NodeId::NodeId( const NodeId& x )ι:NodeId{(UA_NodeId&)x}{}
	NodeId::NodeId( const UA_NodeId& x )ι{
		UA_NodeId_copy( &x, this );
	}

	NodeId::NodeId( UA_NodeId&& x )ι:
		UA_NodeId{ move(x) }{
		UA_NodeId_init( &x );
	}

	NodeId::NodeId( NodeId&& x )ι:NodeId{ (UA_NodeId&&)x }{}

	NodeId::NodeId( const jvalue& j )ε:
		UA_NodeId{ FromJson(j) }
	{}

	NodeId::NodeId( DB::Row& r, uint8 index )ε{
		namespaceIndex = r.Get<uint16>( index );
		if( !r.IsNull(index+1) ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			identifier.numeric = r.Get<UA_UInt32>( index+1 );
		}
		else if( !r.IsNull(index+2) ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			identifier.string = UA_String_fromChars( r.GetString(index+2).c_str() );
		}
		else if( !r.IsNull(index+3) ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			let guid = r.GetGuid( index+3 );
			::memcpy( &identifier.guid, &guid, sizeof(UA_Guid) );
		}
		else if( !r.IsNull(index+4) ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			auto bytes = r.GetBytes( index+4 );
			UA_ByteString_allocBuffer( &identifier.byteString, bytes.size() );
			::memcpy( identifier.byteString.data, bytes.data(), bytes.size() );
		}
	}

	NodeId::NodeId( const flat_map<string,string>& x )ε:
		UA_NodeId{}{
		if( auto p = x.find("ns"); p!=x.end() )
			namespaceIndex = Str::TryTo<UA_UInt16>( p->second ).value_or( 0 );
		if( auto p = x.find("s"); p!=x.end() ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			identifier.string = UA_String_fromChars( p->second.c_str() );
		}
		else if( auto p = x.find("i"); p!=x.end() ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			identifier.numeric = stoul( p->second );
		}
		else if( auto p = x.find("b"); p!=x.end() ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			auto t = ToUV( p->second );
			UA_ByteString_fromBase64( &identifier.byteString, &t );
		}
		else if( auto p = x.find("g"); p!=x.end() ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			ToGuid( p->second, identifier.guid );
		}
		else
			DBGT( ELogTags::App, "No identifier in nodeId" );
	}
	α NodeId::ParseQL( const QL::TableQL& q )ε->vector<NodeId>{
		vector<NodeId> y;
		if( auto v = q.FindPtr<jvalue>( "id" ); v!=nullptr && v->is_array() ){
			for( auto& item : v->get_array() )
				y.emplace_back( item.as_object() );
		}
		else if( v && v->is_object() )
			y.emplace_back( v->get_object() );
		return y;
	}
	α NodeId::operator=( const NodeId& x )ι->NodeId&{
		if( this!=&x ){
			UA_NodeId_clear( this );
			UA_NodeId_copy( &x, this );
		}
		return *this;
	}
	α NodeId::operator=( NodeId&& x )ι->NodeId&{
		if( this!=&x ){
			UA_NodeId_clear( this );
			namespaceIndex = x.namespaceIndex;
			identifierType = x.identifierType;
			identifier = x.identifier;
			UA_NodeId_init( &x );
		}
		return *this;
	}
	α NodeId::operator<( const NodeId& x )Ι->bool{
		return
			namespaceIndex==x.namespaceIndex ?
				identifierType==x.identifierType ?
					identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC ? identifier.numeric<x.identifier.numeric :
						identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING ? ToSV(identifier.string)<ToSV(x.identifier.string) :
						identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING ? ToSV(identifier.byteString)<ToSV(x.identifier.byteString)
					: memcmp( &identifier.guid, &x.identifier.guid, sizeof(UA_Guid) )<0
				: identifierType<x.identifierType
			: namespaceIndex<x.namespaceIndex;
	}

	α NodeId::FromJson( const jobject& j, UA_UInt16 ns )ε->UA_NodeId{
		UA_NodeId nodeId{ ns };
		if( auto p = j.find("ns"); p!=j.end() && p->value().is_number() )
			nodeId.namespaceIndex = Json::AsNumber<UA_UInt16>( p->value() );

		if( auto p = j.find("id"); p!=j.end() )
			return FromJson( p->value(), nodeId.namespaceIndex );
		else if( auto p = j.find("s"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			nodeId.identifier.string = UA_String_fromChars( Json::AsString(p->value()).c_str() );
		}
		else if( auto p = j.find("i"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			nodeId.identifier.numeric = Json::AsNumber<UA_UInt32>( p->value() );
		}
/*		else if( auto p = j.find("number"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			nodeId.identifier.numeric = Json::AsNumber<UA_UInt32>( p->value() );
		}*/
		else if( auto p = j.find("b"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			let v = ToUV( Json::AsSV(p->value()) );
			UA_ByteString_fromBase64( &nodeId.identifier.byteString, &v );
		}
		else if( auto p = j.find("g"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			ToGuid( Json::AsString(p->value()), nodeId.identifier.guid );
		}
		return nodeId;
	};

	α NodeId::FromJson( const jvalue& v, UA_UInt16 ns )ε->UA_NodeId{
		UA_NodeId nodeId{ ns };
		if( v.is_object() )
			nodeId = FromJson( v.get_object(), ns );
		else if( v.is_number() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			nodeId.identifier.numeric = Json::AsNumber<UA_UInt32>( v );
		}
		else if( v.is_string() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			nodeId.identifier.string = UA_String_fromChars( string{v.get_string()}.c_str() );
		}
		else
			THROW( "Could not parse nodeId: {}", serialize(v) );
		return nodeId;
	}

	α NodeId::InsertParams()Ι->vector<DB::Value>{
		vector<DB::Value> params; params.reserve( 64 );
		using enum UA_NodeIdType;
		params.emplace_back( namespaceIndex );
		params.emplace_back( IsNumeric() ? DB::Value{*Numeric()} : DB::Value{} );
		params.emplace_back( IsString() ? DB::Value{*String()} : DB::Value{} );
		params.emplace_back( IsGuid() ? DB::Value{*Guid()} : DB::Value{vector<uint8_t>{}} );
		params.emplace_back( IsBytes() ? DB::Value{FromByteString(*Bytes())} : DB::Value{vector<uint8_t>{}} );
		return params;
	}

	α NodeId::IsSystem( const UA_NodeId& id )ι->bool{ return !id.namespaceIndex && id.identifierType==UA_NODEIDTYPE_NUMERIC && id.identifier.numeric<=32750; }

	α NodeId::ToJson()Ι->jobject{
		return Opc::ToJson( *this );
	}
	α NodeId::ToString()Ι->string{
		UAString j{ 1024 };
		UA_EncodeJsonOptions options{};
		if( let sc=UA_encodeJson( dynamic_cast<const UA_NodeId*>(this), &UA_TYPES[UA_TYPES_NODEID], &j, &options); sc )
			return serialize( ToJson() );
		let y = Opc::ToString( j );
		return y.size()>1 ? y.substr( 1, y.size()-2 ) : y; //remove quotes
	}
	α NodeId::ToString( const vector<NodeId>& nodeIds )ι->string{
		jarray j;
		for( let& nodeId : nodeIds )
			j.push_back( nodeId.ToJson() );
		return serialize( j );
	}

	α toJson( jobject& j, const UA_NodeId& nodeId )ι->jobject{
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
	α NodeId::Add( jobject& j )Ι->void{
		toJson( j, *this );
	}
}
namespace Jde{
	α Opc::ToJson( const UA_NodeId& nodeId )ι->jobject{
		jobject j;
		toJson( j, nodeId );
		return j;
	}
}