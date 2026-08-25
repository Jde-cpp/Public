#include <jde/opc/uatypes/NodeId.h>
#include <jde/db/Row.h>
#include <jde/ql/types/Parser.h>
#include <jde/ql/types/TableQL.h>
#include <jde/opc/UAException.h>

#define let const auto
namespace Jde::Opc{
	NodeId::NodeId( const NodeId& x )ι:NodeId{ (UA_NodeId&)x }{}
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

	NodeId::NodeId( DB::Row& r, uint8 index )ε:
		UA_NodeId{}{
		namespaceIndex = r.Get<uint16>( index );
		if( !r.IsNull(index+1) ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			identifier.numeric = r.Get<UA_UInt32>( index+1 );
		}
		else if( !r.IsNull(index+2) ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			identifier.string = AllocUAString( r.GetString(index+2) );
		}
		else if( !r.IsNull(index+3) ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			identifier.guid = ToUAGuid( r.GetGuid(index+3) );
		}
		else if( !r.IsNull(index+4) ){
			identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			auto bytes = r.GetBytes( index+4 );
			UA_ByteString_allocBuffer( &identifier.byteString, bytes.size() );
			::memcpy( identifier.byteString.data, bytes.data(), bytes.size() );
		}
	}

	NodeId::NodeId( const QL::TableQL& q )ε:
		UA_NodeId{ FromJson(q.As<jvalue>("id")) }
	{}

	α NodeId::ParseQL( const QL::TableQL& q )ε->vector<NodeId>{
		vector<NodeId> y;
		if( auto v = q.FindPtr<jvalue>("id"); v ){
			if( v->is_array() ){
				y.reserve( v->get_array().size() );
				for( let& item : v->get_array() )
					y.emplace_back( item );
			}
			else
				y.emplace_back( *v );
		}
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
	α NodeId::Move()ι->UA_NodeId{
		UA_NodeId y = *this; //slices deliberately: a shallow copy of the base *is* the transfer.
		UA_NodeId_init( this );
		return y;
	}
	α NodeId::operator<( const NodeId& x )Ι->bool{
		return
			namespaceIndex==x.namespaceIndex ?
				identifierType==x.identifierType ?
					identifierType==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC ? identifier.numeric<x.identifier.numeric :
						identifierType==UA_NodeIdType::UA_NODEIDTYPE_STRING ? ToSV( identifier.string )<ToSV( x.identifier.string ) :
						identifierType==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING ? ToSV( identifier.byteString )<ToSV( x.identifier.byteString )
					: memcmp( &identifier.guid, &x.identifier.guid, sizeof(UA_Guid) )<0
				: identifierType<x.identifierType
			: namespaceIndex<x.namespaceIndex;
	}

	α NodeId::FromJson( const jobject& j, UA_UInt16 ns )ε->UA_NodeId{
		UA_NodeId nodeId{ ns };
		if( auto p = j.find("ns"); p!=j.end() )
			nodeId.namespaceIndex = Json::AsNumber<UA_UInt16>( p->value() );

		if( auto p = j.find("id"); p!=j.end() )
			return FromJson( p->value(), nodeId.namespaceIndex );
		else if( auto p = j.find("s"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_STRING;
			nodeId.identifier.string = AllocUAString( Json::AsString(p->value()) );
		}
		else if( auto p = j.find("i"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_NUMERIC;
			nodeId.identifier.numeric = Json::AsNumber<UA_UInt32>( p->value() );
		}
		else if( auto p = j.find("b"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING;
			let v = ToUV( Json::AsSV(p->value()) );
			if( let sc = UA_ByteString_fromBase64(&nodeId.identifier.byteString, &v); sc ){
				UA_ByteString_clear( &nodeId.identifier.byteString );//nodeId is abandoned by the throw, so clear whatever was allocated.
				throw UAException{ sc, Ƒ("Could not base64-decode byte-string node id '{}'.", Json::AsSV(p->value())) };
			}
		}
		else if( auto p = j.find("g"); p!=j.end() ){
			nodeId.identifierType = UA_NodeIdType::UA_NODEIDTYPE_GUID;
			ToGuid( Json::AsString(p->value()), nodeId.identifier.guid );
		}
		else //Loudly:  an object with none of these fell through every branch and returned the zero-initialised
			THROW( "No identifier ('id', 's', 'i', 'b' or 'g') in nodeId: {}", serialize(j) );//{ns,NUMERIC,0} - a real node on many servers, and the wrong one on all of them.
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
			nodeId.identifier.string = AllocUAString( v.get_string() );
		}
		else
			THROW( "Could not parse nodeId: {}", serialize(v) );
		return nodeId;
	}

	α NodeId::InsertParams()Ι->vector<DB::Value>{
		vector<DB::Value> params; params.reserve( 5 );
		using enum UA_NodeIdType;
		params.emplace_back( namespaceIndex );
		params.emplace_back( IsNumeric() ? DB::Value{*Numeric()} : DB::Value{} );
		params.emplace_back( IsString() ? DB::Value{*String()} : DB::Value{} );
		params.emplace_back( IsGuid() ? DB::Value{*Guid()} : DB::Value{} );
		params.emplace_back( IsBytes() ? DB::Value{FromByteString(*Bytes())} : DB::Value{} );
		return params;
	}

	α NodeId::IsSystem( const UA_NodeId& id )ι->bool{ return !id.namespaceIndex && id.identifierType==UA_NODEIDTYPE_NUMERIC && id.identifier.numeric<=32750; }

	α NodeId::ToJson()Ι->jobject{
		return Opc::ToJson( *this );
	}
	α NodeId::ToString()Ι->string{
		UAString s;//empty: the printer allocates and sizes it (a pre-sized buffer would be a cap - review2 #11).
		if( let sc = UA_NodeId_print(static_cast<const UA_NodeId*>(this), &s); sc )
			return serialize( ToJson() );
		return s.ToString();
	}
	α NodeId::ToString( const vector<NodeId>& nodeIds )ι->string{
		jarray j;
		for( let& nodeId : nodeIds )
			j.push_back( nodeId.ToJson() );
		return serialize( j );
	}

	α AddNodeId( jobject& j, const UA_NodeId& nodeId, bool omitDefaultNs )ι->jobject&{
		if( nodeId.namespaceIndex || !omitDefaultNs )
			j["ns"] = nodeId.namespaceIndex;
		const UA_NodeIdType type = nodeId.identifierType;
		if( type==UA_NodeIdType::UA_NODEIDTYPE_NUMERIC )
			j["i"] = nodeId.identifier.numeric;
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_STRING )
			j["s"] = ToSV( nodeId.identifier.string );
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_GUID )
			j["g"] = ToJson( nodeId.identifier.guid );
		else if( type==UA_NodeIdType::UA_NODEIDTYPE_BYTESTRING )
			j["b"] = ByteStringToBase64( nodeId.identifier.byteString );//base64, matching the parser and OPC-UA.  Values still use hex - see ByteStringToJson.
		return j;
	}
	α NodeId::Add( jobject& j )Ι->void{
		AddNodeId( j, *this, false );
	}

	//The UA spelling is `[ns=<n>;]<i|s|g|b>=<identifier>`, and an identifier may legally contain ':' - `urn:plant:line1`,
	//`Channel1.Device1:Tag`, anything with a time - so the branch has to be picked on the *prefix*.  It used to be picked
	//on a ':' anywhere in the string: every such id took the QL branch, where it either threw (OpcAuthorize logs and
	//drops that permission, and the node then falls back to EAccess::All) or, for a json-shaped tail like `ns=2;s=Tag:5`,
	//parsed cleanly as {"ns=2;s=Tag":5} and came back as ns=0;i=0 with no log at all.
	Ω isUaSpelling( sv x )ι->bool{
		if( x.starts_with("ns=") ){
			let semicolon = x.find( ';' );
			if( semicolon==sv::npos )
				return false;
			x = x.substr( semicolon+1 );
		}
		return x.size()>2 && x[1]=='=' && (x[0]=='i' || x[0]=='s' || x[0]=='g' || x[0]=='b');
	}

	α NodeId::DecodeJson( const string& json )ε->NodeId{
		if( !isUaSpelling(json) && json.find(':')!=string::npos ){ //ns:4,i:5002, or the braced {ns:2,s:"tag.one"}
			return FromJson( QL::Parser::ParseArgs(json.starts_with('{') ? json : "{" + json + "}") );
		}
		//ns=4;i=5002 - ToString's own output, and the form an ACL criteria is written in.  UA_NodeId_parse reads it
		//directly; going through UA_decodeJson meant quoting the text back into a json string first, which escaped any
		//", \, tab or newline in the identifier a second time and decoded to a different node.
		NodeId nodeId;
		//static_cast, not &nodeId: NodeId is polymorphic, so the implicit conversion would hand the parser the vptr and
		//it would write its 24-byte image over it.
		if( let sc = UA_NodeId_parse(static_cast<UA_NodeId*>(&nodeId), ToUV(json)); sc )
			throw UAException{ sc, Ƒ("Could not parse NodeId: '{}'", json) };
		return nodeId;
	}
}
namespace Jde{
	α Opc::ToJson( const UA_NodeId& nodeId )ι->jobject{
		jobject j;
		AddNodeId( j, nodeId, false );
		return j;
	}
}