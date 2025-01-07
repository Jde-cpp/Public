#pragma once
#include "../usings.h"
#include "TableQL.h"
#include "MutationQL.h"

namespace Jde::QL{
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