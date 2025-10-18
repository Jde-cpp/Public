#pragma once
#include <jde/fwk/io/json.h>
#include "../exports.h"
#include "Logger.h"

namespace Jde::Opc{
	struct ΓOPC Value : UA_DataValue{
		Value( StatusCode sc )ι:UA_DataValue{ .status{sc}, .hasStatus{true} }{}
		Value( const UA_DataValue& x )ι{ UA_DataValue_copy( &x, this ); }
		Value( UA_DataValue&& x )ι:UA_DataValue{x}{ UA_DataValue_init(&x); }
		Value( const Value& x )ι{ UA_DataValue_copy( &x, this ); }
		Value( Value&& x )ι:UA_DataValue{x}{ UA_DataValue_init(&x); }
		~Value(){ UA_DataValue_clear(this); }

		α operator=( Value&& x )ι->Value&{ UA_DataValue_copy( &x, this ); return *this; }
		α IsEmpty()Ι->bool{ return UA_Variant_isEmpty(&value); }
		α IsScaler()Ι->bool{ return UA_Variant_isScalar( &value ); }
		α ToJson()Ι->jvalue;
		α Set( const jvalue& j, SRCE )ε->void;
		Ŧ Get( uint index )Ι->const T&{ return ((T*)value.data)[index]; };
	private:
		Ŧ SetNumber( const jvalue& j )ε->void;
	};

	Ŧ Value::SetNumber( const jvalue& j )ε->void{
//		if( UA_Variant_isScalar(&value) ){
			T v = Json::AsNumber<T>( j );
			UA_Variant_setScalarCopy( &value, &v, value.type );
//		}
//		else
//			throw Exception( "Arrays Not implemented." );
	}
}