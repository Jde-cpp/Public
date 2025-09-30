#pragma once
#include <jde/access/usings.h>
#include <jde/fwk/co/Await.h>

namespace Jde::DB{ struct Row; }
namespace Jde::Access{
	struct Resource{
		Resource()ι=default;
		Resource( ResourcePK pk, jobject j )ι;
		Resource( DB::Row&& row )ι;
		Resource( jobject j )ι;
		Access::ResourcePK PK;
		string Schema;
		string Target;
		string Filter;
		optional<TimePoint> IsDeleted;
	};
}