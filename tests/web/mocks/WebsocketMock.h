#pragma once
#include <jde/http/usings.h>
#include <jde/web/flex/IWebsocketSession.h>
#include <jde/appClient/Sessions.h>
#include <web/proto/test.pb.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Flex;
	using namespace Jde::Http;
	struct WebsocketSession /*abstract*/ : TWebsocketSession<Proto::FromServerTransmission,Proto::FromClientTransmission>{
		using base = TWebsocketSession<Proto::FromServerTransmission,Proto::FromClientTransmission>;
		WebsocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint )ε : base{ move(stream), move(buffer), move(request), move(userEndpoint) }{}
		α OnRead( Proto::FromClientTransmission&& transmission )ι->void override;
	private:
		α WriteException( const IException& e )ι->void;
		α OnConnect( SessionPK sessionId, Http::RequestId requestId )ι->App::Client::UpsertAwait::Task;
	};
}