#include <jde/ql/types/Subscription.h>

namespace Jde::QL{
	Subscription::Subscription( string tableName, EMutationQL type, TableQL fields )ι:
		Fields{fields}, TableName{tableName}, Type{type}{
		if( auto subscriptionId = Fields.TryNumber<SubscriptionId>("subscriptionId"); subscriptionId ){
			Id = *subscriptionId;
			Fields.Args.erase( "subscriptionId" );
		}
	}
}