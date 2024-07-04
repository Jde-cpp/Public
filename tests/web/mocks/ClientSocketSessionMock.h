#pragma once
#include <jde/http/IClientSocketSession.h>
#include <web/proto/test.pb.h>
#include "ClientSocketAwait.h"

namespace Jde::Web::Mock{
	using namespace Jde::Http;
	struct ClientSocketSession final : TClientSocketSession<Http::Proto::TestFromClient,Http::Proto::TestFromServer>{
		using base = TClientSocketSession<Http::Proto::TestFromClient,Http::Proto::TestFromServer>;
		ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;

		α Connect( SessionPK sessionId, SRCE )ι->ClientSocketAwait<Proto::FromServer::Ack>;
		α Echo( str x, SRCE )ι->ClientSocketAwait<Proto::Echo>;
		α CloseServerSide( SRCE )ι->ClientSocketAwait<Proto::Echo>;
		α BadTransmissionClient( SRCE )ι->ClientSocketAwait<Proto::Echo>;
		α BadTransmissionServer( SRCE )ι->ClientSocketAwait<Proto::Echo>;
	private:
		α HandleException( std::any&& h, string&& what )ι;
		α OnRead( Http::Proto::TestFromServer&& transmission )ι->void override;
		α OnClose( beast::error_code ec )ι->void override;
		α OnAck( Proto::FromServer::Ack&& connect )ι;
	};
}