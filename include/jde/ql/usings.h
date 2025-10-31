#pragma once

namespace Jde::QL{
	enum class EMutationQL : uint8{ Create=0, Update=1, Delete=2, Restore=3, Purge=4, Add=5, Remove=6, Start=7, Stop=8, Execute=9 };
	using ListenerId = uint32;
	using SubscriptionId = uint32;

	struct IListener{
		IListener( str name )ι:Name{name}{}
		β OnChange( const jvalue& j, SubscriptionId clientId )ε->void=0;
		flat_set<SubscriptionId> Ids;
		string Name;
	};
}