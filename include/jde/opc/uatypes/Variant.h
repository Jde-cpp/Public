#pragma once
#include <jde/opc/usings.h>

namespace Jde::DB{ struct Value; }
namespace Jde::Opc{
	struct Variant : UA_Variant{
		//dataType null infers the UA type from the json kind; otherwise the json is decoded *toward* the declared type.
		//A `sv` name used to be taken here and only "double" was honoured - resolving the name is the caller's job (see
		//OpcServer's DT()), since the vocabulary is its config schema, not a UA concept.
		Variant( const jvalue& v, const UA_DataType* dataType )ε;
		Variant( uint32 pk, tuple<uint,void*> data, tuple<UA_UInt32*, uint> dims, const UA_DataType& dataType )ι;
		Variant( Variant&& v )ι;
		Variant( const Variant& v )ι;
		Variant( const UA_Variant& v )ι;
		Variant( UA_Variant&& v )ι;
		Variant( StatusCode sc )ι;
		~Variant(){ UA_Variant_clear(this);  }

		α operator=( const Variant& v )ι->Variant&;
		α operator=( Variant&& v )ι->Variant&;
		α Move()ι->UA_Variant;

		α IsNull()Ι->bool{ return UA_Variant_isEmpty(this); }
		α IsScalar()Ι->bool{ return UA_Variant_isScalar(this); }
		//-> (arrayLength, data); arrayLength 0 is a scalar.  isArray is the caller's persisted shape - the element count
		//cannot tell a scalar from a one-element array, and guessing scalar produced data open62541 rejects against an
		//array-typed node.  Pass true when the stored arrayDimensions are non-empty.
		Ω ToUAValues( const UA_DataType& type, flat_map<uint, string>&& values, bool isArray )ι->tuple<uint,void*>;
		Ω ToArrayDims( str csv )ι->tuple<UA_UInt32*, uint>;

		α ToJson( bool trimNames )ε->jvalue;
		Ω ElementToJson( const void* element, const UA_DataType& type, bool trimNames )ε->jvalue;
		α ToUAJson()ε->vector<string>;
		α ArrayDimString()ι->string;
		α ArrayDimValue()ι->DB::Value;

		VariantPK VariantPK{};
	private:
		Ω GetUAValue( const UA_DataType& type, UA_ByteString j )ε->void*;
	};
}