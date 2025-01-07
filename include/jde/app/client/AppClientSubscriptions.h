#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/types/Subscription.h>
#include <jde/web/client/socket/ClientSocketAwait.h>

namespace Jde::App::Client{
	struct AppClientSocketSession;
	struct ClientSubscriptionAwait final : TAwait<RequestId>{
		ClientSubscriptionAwait( sp<AppClientSocketSession> session, string&& query, sp<QL::IListener> listener, QL::SubscriptionClientId clientId, SRCE )ε:
			TAwait<RequestId>{sl}, _clientId{clientId}, _listener{listener}, _query{move(query)}, _session{session}{}
		α Suspend()ι->void override{ Execute(); }
	private:
		α Execute()ι->Web::Client::ClientSocketAwait<vector<QL::SubscriptionId>>::Task;

		QL::SubscriptionClientId _clientId;
		sp<QL::IListener> _listener;
		string _query;
		sp<AppClientSocketSession> _session;
	};
}