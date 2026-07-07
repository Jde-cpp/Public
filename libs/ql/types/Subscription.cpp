#include <jde/ql/types/Subscription.h>

namespace Jde::QL{
	std::atomic<SubscriptionId> _nextId{ 0x80000000 };//above client-supplied & requestId ranges.
	α Subscription::NextId()ι->SubscriptionId{ return _nextId++; }

	Subscription::Subscription( string tableName, EMutationQL type, TableQL fields )ι:
		Fields{fields}, TableName{tableName}, Type{type}{
		if( auto subscriptionId = Fields.TryNumber<SubscriptionId>("subscriptionId"); subscriptionId ){
			Id = *subscriptionId;
			Fields.Args.erase( "subscriptionId" );
		}
	}
}