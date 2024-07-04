#include "WebsocketMock.h"
#include <jde/web/flex/IWebsocketSession.h>
#include "ServerMock.h"


namespace Jde::Web::Mock{
	α WebsocketSession::OnConnect( SessionPK sessionId, Http::RequestId requestId )ι->Sessions::UpsertAwait::Task{
		try{
			Proto::TestFromServer t;
			auto p = t.add_messages()->mutable_ack();
			auto info = co_await Sessions::UpsertAwait( Jde::format("{:x}", sessionId), _userEndpoint, true );
			p->set_session_id( info.SessionId  );
			p->set_request_id( requestId );
			Write( move(t) );
		}
		catch( IException& e ){
			Proto::TestFromServer t;
			auto p = t.add_messages()->mutable_exception();
			p->set_request_id( requestId );
			p->set_message( e.what() );
			Write( move(t) );
		}
	}
	α WebsocketSession::WriteException( const IException& e )ι->void{
		Proto::TestFromServer t;
		auto p = t.add_messages()->mutable_exception();
		p->set_message( e.what() );
		Write( move(t) );
	}

	α WebsocketSession::OnRead( Proto::TestFromClient&& messages )ι->void{
		for( const Proto::TestFromClientUnion& m : messages.messages() ){
			using enum Proto::TestFromClientUnion::ValueCase;
			switch( m.Value_case() ){
			case kConnect:
				OnConnect( m.connect().session_id(), m.connect().request_id() );
			break;
			case kEcho:{
				//Proto::TestFromServerUnion result;
				Proto::TestFromServer t;
				auto p = t.add_messages()->mutable_echo();
				p->set_request_id( m.echo().request_id() );
				p->set_echo_text( m.echo().echo_text() );
				Write( move(t) );
				break;
			}
			case kCloseServerSide:
				Close();
				break;
			case kBadTransmissionServer:
				Stream->Write( "ABCDEFG" );
				break;
			default:
				BREAK;
				break;
			}
		}
	}
	α RequestHandler::RunWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint )ι->void{
		auto pSession = ms<WebsocketSession>( move(stream), move(buffer), move(req), move(userEndpoint) );
		pSession->Run();
	};

}