#pragma once

namespace Jde::Opc::Server {
	using OAttrPK = uint32;
	struct ObjectAttr final : UA_ObjectAttributes {
		ObjectAttr( DB::Row&& r )ι:
			UA_ObjectAttributes{
				r.GetOpt<UA_UInt32>(1).value_or(0),
				UA_LocalizedText{ "en-US"_uv, ToUV(r.GetString(2)) },
				UA_LocalizedText{ "en-US"_uv, ToUV(r.GetString(3)) },
				r.GetOpt<UA_UInt32>(4).value_or(0),
				r.GetOpt<UA_UInt32>(5).value_or(0),
				r.GetUInt8Opt(6).value_or(0)
			}
		{}
	};
}