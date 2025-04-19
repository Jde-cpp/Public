#pragma once
#include <jde/web/client/usings.h>
#include <jde/web/server/IWebsocketSession.h>
#include <jde/web/server/Sessions.h>
#include "../proto/test.pb.h"

namespace Jde::Web::Mock{
	using namespace Jde::Web::Server;
	using namespace Jde::Web::Client;
	struct ServerSocketSession : TWebsocketSession<Proto::FromServerTransmission,Proto::FromClientTransmission>{
		using base = TWebsocketSession<Proto::FromServerTransmission,Proto::FromClientTransmission>;
		ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α OnRead( Proto::FromClientTransmission&& transmission )ι->void override;
		α SendAck( uint32 serverSocketId )ι->void override;
	private:
		α WriteException( exception&& e, RequestId requestId )ι->void override;
		α WriteException( IException&& e )ι->void{ WriteException( move(e), 0 ); }
		α WriteSubscription( const jvalue& j, RequestId requestId )ι->void override{ ASSERT(false); }
		α WriteSubscriptionAck( vector<QL::SubscriptionId>&& subscriptionIds, RequestId requestId )ι->void override{ ASSERT(false); }
		α WriteComplete( RequestId requestId )ι->void override{ ASSERT(false); }
		α OnConnect( SessionPK sessionId, RequestId requestId )ι->Server::Sessions::UpsertAwait::Task;
	};
}