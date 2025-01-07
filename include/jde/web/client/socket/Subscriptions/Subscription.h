#pragma once
#include "TableQL.h"
#include "MutationQL.h"

namespace Jde::Web::Client{
	using ListenerId = uint32;
	using SubscriptionClientId = uint;
	using SubscriptionId = uint32;

	struct IListener{
		β OnChange( const jvalue& j, SubscriptionClientId clientId )ε->void=0;
		vector<SubscriptionId> Ids;
	};

	struct Subscription{
		Subscription( string tableName, EMutationQL type, TableQL fields )ι:
			Fields{fields}, TableName{tableName}, Type{type}
		{}
		SubscriptionId Id{};
		TableQL Fields;
		string TableName;
		EMutationQL Type;
		flat_map<sp<IListener>,flat_set<SubscriptionClientId>> Listeners;
	};
}