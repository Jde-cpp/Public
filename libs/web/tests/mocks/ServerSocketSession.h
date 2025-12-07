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
		α Schemas()Ι->const vector<sp<DB::AppSchema>>& override{ return _schemas; }
	private:
		α WriteException( exception&& e, RequestId requestId )ι->void override;
		α WriteException( std::string&&, Jde::RequestId )ι->void override{ ASSERT(false); }
		α WriteException( IException&& e )ι->void{ WriteException( move(e), 0 ); }
		α WriteSubscription( const jvalue&, RequestId )ι->void override{ ASSERT(false); }
		β WriteSubscription( uint32 /*appPK*/, uint32 /*appInstancePK*/, const Logging::Entry&, const QL::Subscription& )ι->void override{ ASSERT(false); }
		α WriteSubscriptionAck( flat_set<QL::SubscriptionId>&&, RequestId )ι->void override{ ASSERT(false); }
		α WriteComplete( RequestId )ι->void override{ ASSERT(false); }
		α OnConnect( SessionPK sessionId, RequestId requestId )ι->Server::Sessions::UpsertAwait::Task;

		vector<sp<DB::AppSchema>> _schemas;
	};
}