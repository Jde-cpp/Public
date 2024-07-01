#include "WebsocketMock.h"
#include "ServerMock.h"
#include <jde/web/flex/IWebsocketSession.h>

namespace Jde::Web::Mock{
	using namespace Jde::Web::Flex;

	α WebsocketSession::OnRead( Proto::TestFromClient&& messages )ι->void{
		for( const Proto::TestFromClientUnion& m : messages.messages() ){
			switch( m.Value_case() ){
			case Proto::TestFromClientUnion::ValueCase::kDescription:{
				Proto::TestFromServerUnion result;
				result.set_description( Jde::format("Echo '{}'", m.description()) );
				Proto::TestFromServer message;
				message.add_messages()->CopyFrom( result );
				Write( move(message) );
				break;
			}
			default:
				BREAK;
				break;
			}
		}
	}
	//mutex _sessionMutex;
	//vector<sp<WebsocketSession>> _sessions;
	α RequestHandler::RunWebsocketSession( RestStream&& stream, beast::flat_buffer&& buffer, TRequestType req )ι->void{
		//WebsocketSession ws{ move(stream), move(buffer), move(req) };
		auto pSession = ms<WebsocketSession>( move(stream), move(buffer), move(req) );
		pSession->Run();
		//ul _{ _sessionMutex };
		//_sessions.emplace_back( move(pSession) );

		//pSession->OnRun( make_shared<WebsocketSession>( move(*pSession) ) );
	};
}