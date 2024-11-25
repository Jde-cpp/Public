#pragma once
#include <jde/web/client/socket/IClientSocketSession.h>
#include <tests/proto/test.pb.h>
#include <jde/web/client/socket/ClientSocketAwait.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Client;
	struct ClientSocketSession final : TClientSocketSession<Proto::FromClientTransmission,Proto::FromServerTransmission>{
		using base = TClientSocketSession<Proto::FromClientTransmission,Proto::FromServerTransmission>;
		ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;

		α Connect( SessionPK sessionId, SRCE )ι->ClientSocketAwait<SessionPK>;
		α Echo( str x, SRCE )ι->ClientSocketAwait<string>;
		α CloseServerSide( SRCE )ι->ClientSocketAwait<string>;
		α BadTransmissionClient( SRCE )ι->ClientSocketAwait<string>;
		α BadTransmissionServer( SRCE )ι->ClientSocketAwait<string>;
	private:
		α HandleException( std::any&& h, string&& what )ι;
		α OnRead( Proto::FromServerTransmission&& transmission )ι->void override;
		α OnClose( beast::error_code ec )ι->void override;
		α OnAck( uint32 ack )ι->void;
	};
}