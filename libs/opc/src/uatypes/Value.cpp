#include <jde/opc/uatypes/Value.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/ExNodeId.h>
//#include <jde/opc/uatypes/UAClient.h>

#define let const auto

namespace Jde::Opc{
//#define ADD add.operator()
#define IS(ua) type==&UA_TYPES[ua]
	α Value::ToJson()Ι->jvalue{
		if( status )
			return jobject{ {"sc", status} };
		let scaler = IsScaler();
		let type = value.type;
		jvalue j{ scaler ? jvalue{} : jarray{} };
		auto add = [scaler, &j]( let& v )ι{
			if( scaler )
				j = v;
			else
				j.get_array().push_back( v );
		};
		auto addExplicit = [scaler, &j]( jvalue&& x ){ if( scaler ) j = x; else j.get_array().push_back(x); };
		for( uint i=0; i<(scaler ? 1 : value.arrayLength); ++i ){
			if( IS(UA_TYPES_BOOLEAN) )
				add( Get<UA_Boolean>(i) );
			else if( IS(UA_TYPES_BYTE) )
				addExplicit( (unsigned char)((UA_Byte*)value.data)[i] );
			else if( IS(UA_TYPES_BYTESTRING) ) [[unlikely]]
				addExplicit( ByteStringToJson(((UA_ByteString*)value.data)[i]) );
			else if( IS(UA_TYPES_DATETIME) )
				addExplicit( UADateTime{((UA_DateTime*)value.data)[i]}.ToJson() );
			else if( IS(UA_TYPES_DOUBLE) )
				add( Get<UA_Double>(i) );
			else if( IS(UA_TYPES_DURATION) ) [[unlikely]]
				add( Get<UA_Duration>(i) );
			else if( IS(UA_TYPES_EXPANDEDNODEID) ) [[unlikely]]
				addExplicit( Opc::ToJson( ((UA_ExpandedNodeId*)value.data)[i] ) );
			else if( IS(UA_TYPES_FLOAT) )
				add( Get<UA_Float>(i) );
			else if( IS(UA_TYPES_GUID) ) [[unlikely]]
				addExplicit( Opc::ToJson(((UA_Guid*)value.data)[i]) );
			else if( IS(UA_TYPES_INT16) ) [[likely]]
				add( Get<UA_Int16>(i) );
			else if( IS(UA_TYPES_INT32) ) [[likely]]
				add( Get<UA_Int32>(i) );
			else if( IS(UA_TYPES_INT64) )
				addExplicit( Opc::ToJson(((UA_Int64*)value.data)[i]) );
			else if( IS(UA_TYPES_NODEID) )
				addExplicit( Opc::ToJson((((UA_NodeId*)value.data)[i])) );
			else if( IS(UA_TYPES_SBYTE) )
				addExplicit( (char)((UA_SByte*)value.data)[i] );
			else if( IS(UA_TYPES_STATUSCODE) )
				add( Get<StatusCode>(i) );
			else if( IS(UA_TYPES_STRING) ) [[likely]]
				addExplicit( jstring{ToSV(((UA_String*)value.data)[i])} );
			else if( IS(UA_TYPES_UINT16) )
				add( Get<UA_UInt16>(i) );
			else if( IS(UA_TYPES_UINT32) ) [[likely]]
				add( Get<UA_UInt32>(i) );
			else if( IS(UA_TYPES_UINT64) )
				addExplicit( Opc::ToJson(((UA_UInt64*)value.data)[i]) );
			else if( IS(UA_TYPES_XMLELEMENT) ) [[unlikely]]
				addExplicit( jstring{ToSV(((UA_XmlElement*)value.data)[i])} );
			else{
				WARNT( IotReadTag, "Unsupported type {}.", type->typeName );
				addExplicit( jstring{Ƒ("Unsupported type {}.", type->typeName)} );
			}
		}
		return j;
	}

	α Value::Set( const jvalue& j )ε->void{
		let scaler = UA_Variant_isScalar( &value );
		let type = value.type;
		if( IS(UA_TYPES_BOOLEAN) ){
			if( scaler ){
				THROW_IF( !j.is_bool(), "Expected bool '{}'.", serialize(j) );
				UA_Boolean v = j.get_bool();
				UA_Variant_setScalarCopy( &value, &v, type );
			}
			else
				throw Exception( "Not implemented." );
		}
		else if( IS(UA_TYPES_BYTE) )
			SetNumber<UA_Byte>( j );
		else if( IS(UA_TYPES_DOUBLE) )
			SetNumber<UA_Double>( j );
		else if( IS(UA_TYPES_FLOAT) )
			SetNumber<UA_Float>( j );
		else if( IS(UA_TYPES_INT16) )
			SetNumber<UA_Int16>( j );
		else if( IS(UA_TYPES_INT32) )
			SetNumber<UA_Int32>( j );
		else if( IS(UA_TYPES_INT64) )
			SetNumber<UA_Int64>( j );
		else if( IS(UA_TYPES_STRING) ){
			if( scaler ){
				THROW_IF( !j.is_string(), "Expected string '{}'.", serialize(j) );
				UA_String v = UA_String_fromChars( j.get_string().c_str() );
				UA_Variant_setScalarCopy( &value, &v, type );
			}
		}
		else if( IS(UA_TYPES_UINT16) )
			SetNumber<UA_UInt16>( j );
		else if( IS(UA_TYPES_UINT32) )
			SetNumber<UA_UInt32>( j );
		else if( IS(UA_TYPES_UINT64) )
			SetNumber<UA_UInt64>( j );
		else
			THROW( "Setting type '{}' has not been implemented.", type->typeName );
	}
}