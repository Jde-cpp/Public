#pragma once
#include <jde/web/flex/IWebsocketSession.h>
#include <web/proto/test.pb.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Flex;
	using namespace Jde::Http;
	struct WebsocketSession /*abstract*/ : TWebsocketSession<Proto::TestFromServer,Proto::TestFromClient>{
		WebsocketSession( RestStream&& stream, beast::flat_buffer&& buffer, TRequestType&& request )ε : TWebsocketSession<Proto::TestFromServer,Proto::TestFromClient>{ move(stream), move(buffer), move(request) }{}
		α OnRead( Proto::TestFromClient&& transmission )ι->void override;
	};
}