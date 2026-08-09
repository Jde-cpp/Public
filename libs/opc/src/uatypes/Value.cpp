#include <jde/opc/uatypes/Value.h>
#include <jde/opc/UAException.h>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/DateTime.h>
#include <jde/opc/uatypes/ExNodeId.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Variant.h>


#define let const auto

namespace Jde::Opc{
	α Value::operator=( Value&& x )ι->Value&{
		UA_DataValue_clear(this);
		static_cast<UA_DataValue&>(*this) = static_cast<UA_DataValue&>(x);
		UA_DataValue_init(&x);
		return *this;
	}

#define IS(ua) type==&UA_TYPES[ua]
	α Value::ToJson()Ι->jvalue{
		//Severity, not "non-zero".  Only a Bad code means the reading is unusable; an Uncertain one (0x4xxxxxxx) still
		//carries a value per OPC-UA - UNCERTAINLASTUSABLEVALUE with a last reading of 42.7 used to serialize as {"sc":…}
		//and blank the tag in the UI for a node that was still reporting.
		if( UA_StatusCode_isBad(status) )
			return UAException::ToJson( status );
		if( IsEmpty() )
			return hasStatus ? UAException::ToJson( status ) : jvalue{};//nothing to show, so say why if we were told.
		let scaler = IsScalar();
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
			else if( IS(UA_TYPES_LOCALIZEDTEXT) ) [[unlikely]]
				addExplicit( jstring{ToSV(((UA_LocalizedText*)value.data)[i].text)} ); //the text alone, as the Variant fallback's trimNames did.
			else if( IS(UA_TYPES_QUALIFIEDNAME) ) [[unlikely]]
				addExplicit( BrowseName::ToJson(((UA_QualifiedName*)value.data)[i]) );
			else{
				//Element i, not the whole variant: this used to re-encode every element for each element, which gave an
				//array of N copies of the whole array.  The try is what keeps ToJson's Ι honest - ElementToJson is ε, and
				//the encoder throws on anything it cannot fit (see #11).  A failed element becomes a status object rather
				//than taking the process down: both callers are noexcept, one of them an open62541 C callback.
				try{
					addExplicit( Variant::ElementToJson((const UA_Byte*)value.data + i*type->memSize, *type, true) );
				}
				catch( const exception& e ){
					ERRT( ELogTags::Parsing, "Could not encode element {} of '{}' - {}.  Serializing it as a status instead.", i, type->typeName, e.what() );
					addExplicit( UAException::ToJson(UA_STATUSCODE_BADENCODINGERROR) );
				}
			}
		}
		//A non-zero code rides along with the reading - {"v":…,"sc":…} - rather than replacing it.  Uncertain is the
		//motivating case, but an informative Good sub-code (GoodClamped) is reported too: the rule is "never drop a status".
		return status ? jvalue{ jobject{ {"v", move(j)}, {"sc", status} } } : j;
	}

	α Value::Set( const jvalue& j, SL sl )ε->void{
		auto& type = value.type;
		if( !type ){
			if( j.is_null() )
				throw Exception{ sl, ELogLevel::Error, "Value has no type and json is null." };
			else if( j.is_bool() )
				type = &UA_TYPES[UA_TYPES_BOOLEAN];
			else if( j.is_int64() )
				type = &UA_TYPES[UA_TYPES_INT64];
			else if( j.is_uint64() )
				type = &UA_TYPES[UA_TYPES_UINT64];
			else if( j.is_double() )
				type = &UA_TYPES[UA_TYPES_DOUBLE];
			else if( j.is_string() )
				type = &UA_TYPES[UA_TYPES_STRING];
			else
				throw Exception{ sl, ELogLevel::Error, "Value has no type." };
		}
		let dt = type;
		UA_Variant_clear( &value );
		type = dt;
		auto setDuration = [&]()ε->void {
			let& o = j.as_object();
			THROW_IF( !o.contains("seconds") || !o.contains("nanos"), "Expected duration object with 'seconds' and 'nanos' - '{}'.", serialize(j) );
			//Scale in double instead of adding chrono durations:  seconds+nanoseconds has int64 *nanoseconds* as its common
			//type, so a client-supplied `seconds` past ~292 years overflowed the conversion - UB, and a UBSan abort in debug.
			//The fields keep their integer reads, so an out-of-range or non-integral one still throws rather than truncating.
			//Same formula as the socket's toDuration, so both entry points into a UA_Duration agree.
			SetNumber<UA_Duration>( o.at("seconds").to_number<int64_t>()*1000. + o.at("nanos").to_number<int32_t>()/1'000'000. );
		};

		if( IS(UA_TYPES_BOOLEAN) ){
			THROW_IF( !j.is_bool(), "Expected bool '{}'.", serialize(j) );
			UA_Boolean v = j.get_bool();
			UA_Variant_setScalarCopy( &value, &v, type );
		}
		else if( IS(UA_TYPES_BYTE) )
			SetNumber<UA_Byte>( j );
		//One branch for both:  UA_Duration *is* UA_Double, a count of milliseconds, and either can arrive in either json
		//spelling.  ToJson emits the bare number, while the socket sends a Duration as a google.protobuf.Duration, which
		//reaches a client as {seconds,nanos}.  DURATION used to accept only the object, so writing back a value the client
		//had just read over REST threw inside as_object().
		else if( IS(UA_TYPES_DOUBLE) || IS(UA_TYPES_DURATION) ){
			if( j.is_object() )
				setDuration();
			else
				SetNumber<UA_Double>( j );//value.type is untouched, so a DURATION stays a DURATION.
		}
		else if( IS(UA_TYPES_FLOAT) )
			SetNumber<UA_Float>( j );
		else if( IS(UA_TYPES_INT16) )
			SetNumber<UA_Int16>( j );
		else if( IS(UA_TYPES_INT32) )
			SetNumber<UA_Int32>( j );
		else if( IS(UA_TYPES_INT64) )
			SetNumber<UA_Int64>( j );
		else if( IS(UA_TYPES_STRING) || IS(UA_TYPES_LOCALIZEDTEXT) ){
			THROW_IF( !j.is_string(), "Expected string '{}'.", serialize(j) );
			let str = ToUV( j.get_string() );
			if( IS(UA_TYPES_STRING) )
				UA_Variant_setScalarCopy( &value, &str, type );
			else{
				UA_LocalizedText lt;
				lt.locale = UA_STRING_NULL;
				lt.text = str;
				UA_Variant_setScalarCopy( &value, &lt, type );
			}
		}
		else if( IS(UA_TYPES_UINT16) )
			SetNumber<UA_UInt16>( j );
		else if( IS(UA_TYPES_UINT32) )
			SetNumber<UA_UInt32>( j );
		else if( IS(UA_TYPES_UINT64) )
			SetNumber<UA_UInt64>( j );
		else if( IS(UA_TYPES_DATETIME) || IS(UA_TYPES_UTCTIME) )
			SetNumber<UA_DateTime>( UADateTime{j}.UA() );
		else
			THROW( "Setting type '{}' has not been implemented - {}", type->typeName, serialize(j) );
	}
}