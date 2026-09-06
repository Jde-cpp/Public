#pragma once
#include <jde/ql/types/Subscription.h>

namespace Jde::QL{ struct MutationQL;}
namespace Jde::QL::Subscriptions{
	α StopListen( sp<IListener> listener, vector<SubscriptionId> ids={}, SRCE )ι->jarray;//returns the ids removed; sl is the unsubscriber's, for the trace.
	α Listen( sp<IListener> listener, vector<Subscription>&& subs )ι->void;//ungated - in-process registration only (the AccessListener, System).
	α Listen( sp<IListener> listener, vector<Subscription>&& subs, UserPK executer, SRCE )ε->void;//everything a client asked for.
	α OnMutation( const MutationQL& m, jvalue result )ι->void;
	α OnMutation( const MutationQL& m, jvalue result, function<bool(QL::TableQL&)> isApplicable )ι->void;
}