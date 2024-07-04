#pragma once
#include <jde/http/usings.h>
#include <jde/web/flex/IWebsocketSession.h>
#include <jde/web/flex/Sessions.h>
#include <web/proto/test.pb.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Flex;
	using namespace Jde::Http;
	struct WebsocketSession /*abstract*/ : TWebsocketSession<Proto::TestFromServer,Proto::TestFromClient>{
		WebsocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint )ε : TWebsocketSession<Proto::TestFromServer,Proto::TestFromClient>{ move(stream), move(buffer), move(request), move(userEndpoint) }{}
		α OnRead( Proto::TestFromClient&& transmission )ι->void override;
	private:
		α WriteException( const IException& e )ι->void;
		α OnConnect( SessionPK sessionId, Http::RequestId requestId )ι->Sessions::UpsertAwait::Task;
	};
}