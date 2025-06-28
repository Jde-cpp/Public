#pragma once

#define let const auto

namespace Jde::Opc::Server{
	struct Variant : UA_Variant{
		Variant( const jvalue& v, sv dataType )ι;
		Variant( uint32 pk, tuple<uint,void*> data, tuple<UA_UInt32*, uint> dims, const UA_DataType& dataType )ι;
		Variant( Variant&& v )ι;
		Variant( const Variant& v )ι;
		Variant( const UA_Variant& v )ι;

		α operator=( const Variant& v )ι->Variant&;
		α operator=( Variant&& v )ι->Variant&;

		Ω ToUAValues( const UA_DataType& type, flat_map<uint32, string>&& values )ι->tuple<uint,void*>;
		Ω ToArrayDims( str csv )ι->tuple<UA_UInt32*, uint>;

		α ToJson()ε->flat_map<uint32, string>;
		α ArrayDimString()ι->string;

		VariantPK VariantPK{};
	private:
		Ω GetUAValue( const UA_DataType& type, UA_ByteString j )ε->void*;
	};
}