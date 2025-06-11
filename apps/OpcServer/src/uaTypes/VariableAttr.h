#pragma once

namespace Jde::Opc::Server {
	using VAttrPK = uint32;
	struct VariableAttr final : UA_VariableAttributes{
		VariableAttr( DB::Row& r, UA_Variant&& variant, const UA_DataType& dataType, tuple<UA_UInt32*, uint> dims )ι:
			UA_VariableAttributes{
				r.GetOpt<UA_UInt32>(1).value_or(0),
				UA_LocalizedText{ "en-US"_uv, ToUV(r.GetString(2)) },
				UA_LocalizedText{ "en-US"_uv, ToUV(r.GetString(3)) },
				r.GetOpt<UA_UInt32>(4).value_or(0),
				r.GetOpt<UA_UInt32>(5).value_or(0),
				variant,
				dataType.typeId,
				r.GetOpt<UA_Int32>(8).value_or(0),
				get<1>(dims),
				get<0>(dims),
				r.GetUInt8Opt(10).value_or(0),
				r.GetUInt8Opt(11).value_or(0),
				r.GetDoubleOpt(12).value_or(-1.0),
				r.GetBit(13)
			}
		{}
	};
}