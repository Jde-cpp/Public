#pragma once
#include <jde/http/IClientSocketSession.h>
#include <web/proto/test.pb.h>
#include <jde/http/ClientSocketAwait.h>

namespace Jde::Web::Mock{
	using namespace Jde::Http;
	struct ClientSocketSession final : TClientSocketSession<Http::Proto::FromClientTransmission,Http::Proto::FromServerTransmission>{
		using base = TClientSocketSession<Http::Proto::FromClientTransmission,Http::Proto::FromServerTransmission>;
		ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;

		α Connect( SessionPK sessionId, SRCE )ι->ClientSocketAwait<SessionPK>;
		α Echo( str x, SRCE )ι->ClientSocketAwait<string>;
		α CloseServerSide( SRCE )ι->ClientSocketAwait<string>;
		α BadTransmissionClient( SRCE )ι->ClientSocketAwait<string>;
		α BadTransmissionServer( SRCE )ι->ClientSocketAwait<string>;
	private:
		α HandleException( std::any&& h, string&& what )ι;
		α OnRead( Http::Proto::FromServerTransmission&& transmission )ι->void override;
		α OnClose( beast::error_code ec )ι->void override;
		α OnAck( RequestId requestId, SessionPK sessionId )ι->void;
	};
}