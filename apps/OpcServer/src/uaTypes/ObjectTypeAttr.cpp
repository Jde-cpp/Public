#include "ObjectTypeAttr.h"

namespace Jde::Opc::Server {
	ObjectTypeAttr::ObjectTypeAttr( DB::Row&& r )ι :
		UA_ObjectTypeAttributes{
			r.GetOpt<UA_UInt32>(1).value_or(0),
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(2)) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(r.GetString(3)) },
			r.GetOpt<UA_UInt32>(4).value_or(0),
			r.GetOpt<UA_UInt32>(5).value_or(0),
			r.GetBitOpt(6).value_or(false)
		}
	{}

	ObjectTypeAttr::ObjectTypeAttr( const jobject& j )ι:
		UA_ObjectTypeAttributes{
			Json::FindNumber<UA_UInt32>( j, "specified" ).value_or(0),
			UA_LocalizedText{ "en-US"_uv, AllocUAString(Json::AsString( j, "name" )) },
			UA_LocalizedText{ "en-US"_uv, AllocUAString(Json::FindString(j, "description").value_or("")) },
			Json::FindNumber<UA_UInt32>( j, "writeMask" ).value_or(0),
			Json::FindNumber<UA_UInt32>( j, "userWriteMask" ).value_or(0),
			Json::FindBool( j, "isAbstract" ).value_or(false)
		}
	{}

}