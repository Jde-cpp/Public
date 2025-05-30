#pragma once
#include <jde/ql/usings.h>

namespace Jde::QL{ struct Subscription; }
namespace Jde::Web::Client::Subscriptions{
	α StopListenRemote( sp<QL::IListener> listener, vector<QL::SubscriptionId> ids )ι->jarray;
	α ListenRemote( sp<QL::IListener> listener, vector<QL::Subscription>&& subs )ι->void;
	α OnWebsocketReceive( const jobject& m, QL::SubscriptionId clientId )ι->void;
}