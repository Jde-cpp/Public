#pragma once
#include <jde/ql/types/Subscription.h>

namespace Jde::QL{ struct MutationQL;}
namespace Jde::QL::Subscriptions{
	α StopListen( sp<IListener> listener, vector<SubscriptionId> ids={} )ι->jarray;
	α Listen( sp<IListener> listener, vector<Subscription>&& subs )ι->void;
	α OnMutation( const MutationQL& m, jvalue result )ι->void;
	α OnMutation( const MutationQL& m, jvalue result, function<bool(QL::TableQL&)> isApplicable )ι->void;
}