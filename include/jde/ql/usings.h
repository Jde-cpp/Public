#pragma once
#include <jde/framework/coroutine/Await.h>

namespace Jde::QL{

	enum class EMutationQL : uint8{ Create=0, Update=1, Delete=2, Restore=3, Purge=4, Add=5, Remove=6, Start=7, Stop=8 };
	using ListenerId = uint32;
	using SubscriptionClientId = uint;
	using SubscriptionId = uint32;

	struct IListener{
		β OnChange( const jvalue& j, SubscriptionClientId clientId )ε->void=0;
		vector<SubscriptionId> Ids;
	};
}