#pragma once
#include <jde/web/server/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/types/Subscription.h>
//#include <jde/web/usings.h>


namespace Jde::Web::Server{ struct IWebsocketSession; }
namespace Jde::Web::Server::Subscriptions{
	α Add( string&& query, RequestId requestId, sp<IWebsocketSession> session, SRCE )ι->TAwait<vector<QL::SubscriptionId>>::Task;
	α Remove( vector<RequestId>&& previousRequestIds, RequestId currentRequestId, sp<IWebsocketSession> session, SRCE )ι->VoidAwait<>::Task;
	α Close( SocketId socketId, SRCE )ι->VoidAwait<>::Task;
}
