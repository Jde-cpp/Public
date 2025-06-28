#pragma once

namespace Jde::Opc::Server {
	struct ObjectTypeAttr final : UA_ObjectTypeAttributes {
		ObjectTypeAttr( DB::Row&& r )ι;
		ObjectTypeAttr( const jobject& j )ι;
	};
}