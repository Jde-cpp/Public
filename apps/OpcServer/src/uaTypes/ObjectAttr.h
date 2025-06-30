#pragma once

namespace Jde::Opc::Server {
	struct ObjectAttr final : UA_ObjectAttributes {
		ObjectAttr( const jobject& j )ι:
			UA_ObjectAttributes{
				Json::FindNumber<UA_UInt32>(j, "specified").value_or(0),
				UA_LocalizedText{ "en-US"_uv, AllocUAString(j.at("name").as_string()) },
				UA_LocalizedText{ "en-US"_uv, AllocUAString(Json::FindString(j, "description").value_or("")) },
				Json::FindNumber<UA_UInt32>( j, "writeMask" ).value_or(0),
				Json::FindNumber<UA_UInt32>( j, "userWriteMask").value_or(0),
				Json::FindNumber<UA_Byte>( j, "eventNotifier" ).value_or(0)
			}
		{}
		ObjectAttr( DB::Row&& r )ι:
			UA_ObjectAttributes{
				r.GetOpt<UA_UInt32>(1).value_or(0),
				UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(2)) },
				UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(3)) },
				r.GetOpt<UA_UInt32>(4).value_or(0),
				r.GetOpt<UA_UInt32>(5).value_or(0),
				r.GetUInt8Opt(6).value_or(0)
			},
			PK{ r.GetUInt32(0) }
		{}

		OAttrPK PK{};
	};
}