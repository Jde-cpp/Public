#include "ClientSocketSessionMock.h"

namespace Jde::Web::Mock{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	ClientSocketSession::ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		base{ ioc, ctx }
	{}

	α ClientSocketSession::OnRead( Http::Proto::TestFromServer&& transmission )ι->void{
		for( auto&& m : transmission.messages() ){
			switch( m.Value_case() ){
			case Http::Proto::TestFromServerUnion::ValueCase::kAck:
				HandleAck( m.ack() );
				INFOT( IncomingTag(), "[{:x}]ClientSocketSession Created.", m.ack().session_id() );
				break;
			default:
				BREAK;
			}
		}
	}

}