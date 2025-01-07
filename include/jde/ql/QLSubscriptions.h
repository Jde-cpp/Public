#pragma once
#include <jde/ql/types/Subscription.h>

namespace Jde::QL{ struct MutationQL;}
namespace Jde::QL::Subscriptions{
	α Add( vector<Subscription>&& subs )ι->jarray;
	α Remove( vector<uint>&& ids )ι->jarray;
	α Listen( sp<IListener> listener, SubscriptionId subscriptionId, SubscriptionClientId clientId )ι->void;
	α Push( const MutationQL& mutation, jvalue result )ι->void;
	α Push( const jobject& m, SubscriptionClientId clientId )ι->void;
}
