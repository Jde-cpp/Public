#pragma once
#include <jde/fwk/io/json.h>
#include "Logger.h"

namespace Jde::Opc{
	struct Value : UA_DataValue{
		Value( StatusCode sc )ι:UA_DataValue{ .status=sc, .hasStatus=true }{}
		Value( const UA_DataValue& x )ι{ UA_DataValue_copy( &x, this ); }
		Value( UA_DataValue&& x )ι:UA_DataValue{x}{ UA_DataValue_init(&x); }
		Value( const Value& x )ι{ UA_DataValue_copy( &x, this ); }
		Value( Value&& x )ι:UA_DataValue{x}{ UA_DataValue_init(&x); }
		Value( const jvalue& j, const UA_DataType* dt, SRCE )ε:UA_DataValue{ .value{.type=dt} }{ Set(j, sl); }
		~Value(){ UA_DataValue_clear(this); }

		α operator=( Value&& x )ι->Value&;
		α IsEmpty()Ι->bool{ return UA_Variant_isEmpty(&value); }
		α IsScalar()Ι->bool{ return UA_Variant_isScalar( &value ); }
		α ToJson()Ι->jvalue;
		Ŧ AsNumber( SRCE )ε->T;
		α Set( const jvalue& j, SRCE )ε->void;
		Ŧ Get( uint index )Ι->const T&{ return ((T*)value.data)[index]; };
	private:
		Ŧ SetNumber( const jvalue& j )ε->void;
		template<class T, class U> Ω Narrow( U v, SL sl )ε->T;
	};

	//Straight off value.data, not via ToJson():  that renders INT64/UINT64 as the protobufjs Long form {high,low,unsigned},
	//which Json::AsNumber then throws on, so AsNumber failed for every 64-bit integer - and an empty value reported the
	//misleading "Arrays Not implemented."
	Ŧ Value::AsNumber( SL sl )ε->T{
		THROW_IFSL( IsEmpty(), "Value is empty{}.", hasStatus ? Ƒ(" - status 0x{:x}", (uint32)status) : string{} );
		THROW_IFSL( !IsScalar(), "AsNumber is not implemented for a '{}' array.", value.type->typeName );
		const UA_DataType* type = value.type;//not `let`: that macro is defined per .cpp, not in headers.
		if( type==&UA_TYPES[UA_TYPES_BOOLEAN] )	return Narrow<T>( Get<UA_Boolean>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_SBYTE] )		return Narrow<T>( Get<UA_SByte>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_BYTE] )		return Narrow<T>( Get<UA_Byte>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_INT16] )		return Narrow<T>( Get<UA_Int16>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_UINT16] )	return Narrow<T>( Get<UA_UInt16>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_INT32] )		return Narrow<T>( Get<UA_Int32>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_UINT32] )	return Narrow<T>( Get<UA_UInt32>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_INT64] )		return Narrow<T>( Get<UA_Int64>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_UINT64] )	return Narrow<T>( Get<UA_UInt64>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_STATUSCODE] ) return Narrow<T>( Get<UA_StatusCode>(0), sl );
		if( type==&UA_TYPES[UA_TYPES_FLOAT] )		return (T)Get<UA_Float>( 0 );
		if( type==&UA_TYPES[UA_TYPES_DOUBLE] || type==&UA_TYPES[UA_TYPES_DURATION] ) return (T)Get<UA_Double>( 0 );
		throw Exception{ sl, ELogLevel::Error, "'{}' is not a number.", type->typeName };
	}

	//The json path rejected a value too wide for T; keep that rather than truncating silently.  Convert back and compare
	//rather than std::in_range, which is defined only for the standard integer types and rejects UA_Boolean.  The second
	//test catches a sign flip that survives the round trip, e.g. -1 through an unsigned T.  A float source is converted
	//unchecked, as the json path also did.
	template<class T, class U> α Value::Narrow( U v, SL sl )ε->T{
		const T y = (T)v;//not `let`: that macro is defined per .cpp, not in headers.
		if constexpr( std::is_integral_v<T> && std::is_integral_v<U> )
			THROW_IFSL( (U)y!=v || (y<T{})!=(v<U{}), "{} does not fit the requested type.", v );
		return y;
	}

	Ŧ Value::SetNumber( const jvalue& j )ε->void{
		T v = Json::AsNumber<T>( j );
		UA_Variant_setScalarCopy( &value, &v, value.type );
	}
}