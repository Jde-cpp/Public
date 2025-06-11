#pragma once

#define let const auto

namespace Jde::Opc::Server{
	using VariantPK = uint32;
	struct Variant : UA_Variant{
		Variant( uint32 pk, tuple<uint,void*> data, tuple<UA_UInt32*, uint> dims, const UA_DataType& dataType )ι;
		Variant( UA_Variant&& v )ι;

		Ω ToUAValues( const UA_DataType& type, flat_map<uint32, string>&& values )ι->tuple<uint,void*>;
		Ω ToArrayDims( str csv )ι->tuple<UA_UInt32*, uint>;

		VariantPK VariantPK;
	private:
		Ω GetUAValue( const UA_DataType& type, UA_ByteString j )ε->void*;
	};
}