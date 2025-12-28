#pragma once
#include <jde/ql/usings.h>

namespace Jde::QL{ struct Subscription; }
namespace Jde::App::Proto::FromServer{ struct Traces; }
namespace Jde::App::Client::Subscriptions{
	α StopListenRemote( sp<QL::IListener> listener, vector<QL::SubscriptionId> ids )ι->jarray;
	α ListenRemote( sp<QL::IListener> listener, QL::Subscription&& sub )ι->void;
	α OnTraces( App::Proto::FromServer::Traces&& traces, QL::SubscriptionId requestId )ι->void;
	α OnWebsocketReceive( const jobject& m, QL::SubscriptionId clientId )ι->void;
}