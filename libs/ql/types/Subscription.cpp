#include <jde/ql/types/Subscription.h>

namespace Jde::QL{
	Subscription::Subscription( string tableName, EMutationQL type, TableQL fields, SL sl )ε:
		Fields{fields}, TableName{tableName}, Type{type}{
		if( auto subscriptionId = Fields.Args.find("subscriptionId"); subscriptionId!=Fields.Args.end() ){
			Id = Json::AsNumber<SubscriptionId>( subscriptionId->value(), sl );
			Fields.Args.erase( subscriptionId );
		}
	}
}