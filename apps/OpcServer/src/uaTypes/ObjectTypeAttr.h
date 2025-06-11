#pragma once

namespace Jde::Opc::Server {
	using OTypeAttrPK = UA_UInt32;
	struct ObjectTypeAttr final : UA_ObjectTypeAttributes {
		ObjectTypeAttr( DB::Row&& r )ι :
			UA_ObjectTypeAttributes{
				r.GetOpt<UA_UInt32>(1).value_or(0),
				UA_LocalizedText{ "en-US"_uv, ToUV(r.GetString(2)) },
				UA_LocalizedText{ "en-US"_uv, ToUV(r.GetString(3)) },
				r.GetOpt<UA_UInt32>(4).value_or(0),
				r.GetOpt<UA_UInt32>(5).value_or(0),
				r.GetBitOpt(6).value_or(false)
			}
		{}
	};
}