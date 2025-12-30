#pragma once
#include "../usings.h"
#include "TableQL.h"
#include "MutationQL.h"

namespace Jde::QL{
	struct Subscription{
		Subscription( string tableName, EMutationQL type, TableQL fields, SRCE )ε;
		SubscriptionId Id{};
		TableQL Fields;
		string TableName;
		EMutationQL Type;
	};
}