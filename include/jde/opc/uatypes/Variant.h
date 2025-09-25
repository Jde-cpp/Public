#pragma once
#include <jde/opc/usings.h>

namespace Jde::Opc{
	struct Variant : UA_Variant{
		Variant( const jvalue& v, sv dataType )ι;
		Variant( uint32 pk, tuple<uint,void*> data, tuple<UA_UInt32*, uint> dims, const UA_DataType& dataType )ι;
		Variant( Variant&& v )ι;
		Variant( const Variant& v )ι;
		Variant( const UA_Variant& v )ι;
		Variant( UA_Variant&& v )ι;
		Variant( StatusCode sc )ι;

		α operator=( const Variant& v )ι->Variant&;
		α operator=( Variant&& v )ι->Variant&;

		α IsNull()Ι->bool{ return UA_Variant_isEmpty(this); }
		α IsScalar()Ι->bool{ return UA_Variant_isScalar(this); }
		Ω ToUAValues( const UA_DataType& type, flat_map<uint, string>&& values )ι->tuple<uint,void*>;
		Ω ToArrayDims( str csv )ι->tuple<UA_UInt32*, uint>;

		α ToJson( bool trimNames )ε->jvalue;
		α ToUAJson()ε->vector<string>;
		α ArrayDimString()ι->string;

		VariantPK VariantPK{};
	private:
		Ω GetUAValue( const UA_DataType& type, UA_ByteString j )ε->void*;
	};
}