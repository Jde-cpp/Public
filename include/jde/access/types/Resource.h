#pragma once
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>

namespace Jde::DB{ struct Row; }
namespace Jde::Access{
	struct Resource{
		Resource()ι=default;
		Resource( ResourcePK pk, const jobject& j )ι;
		Resource( DB::Row&& row )ι;
		Resource( const jobject& j )ι;
		Access::ResourcePK PK;
		string Schema;
		string Target;
		string Filter;
		optional<TimePoint> IsDeleted;
	};
}