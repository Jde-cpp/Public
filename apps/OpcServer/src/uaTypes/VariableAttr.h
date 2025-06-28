#pragma once
#include "Node.h"

namespace Jde::Opc::Server {
	struct VariableAttr final : UA_VariableAttributes{
		VariableAttr( jobject& j, SRCE )ε;
		VariableAttr( DB::Row& r, UA_Variant&& variant, const UA_DataType& dataType, tuple<UA_UInt32*, uint> dims )ε;

		Ω ArrayDimensionsString( size_t arrayDimensionsSize, UA_UInt32* arrayDimensions )ι->string;
	};
}