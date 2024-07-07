#include "WebsocketMock.h"
#include <jde/web/flex/IWebsocketSession.h>
#include "ServerMock.h"


namespace Jde::Web::Mock{
	α WebsocketSession::OnConnect( SessionPK sessionId, Http::RequestId requestId )ι->App::Client::UpsertAwait::Task{
		try{
			Proto::FromServerTransmission t;
			auto m = t.add_messages();
			auto info = co_await App::Client::UpsertAwait( Jde::format("{:x}", sessionId), _userEndpoint.address().to_string(), true );
			m->set_ack( info.SessionId );
			m->set_request_id( requestId );
			Write( move(t) );
		}
		catch( IException& e ){
			Proto::FromServerTransmission t;
			auto m = t.add_messages();
			m->set_request_id( requestId );
			m->set_exception( e.what() );
			Write( move(t) );
		}
	}
	α WebsocketSession::WriteException( const IException& e )ι->void{
		Proto::FromServerTransmission t;
		auto m = t.add_messages();
		m->set_exception( e.what() );
		Write( move(t) );
	}

	α WebsocketSession::OnRead( Proto::FromClientTransmission&& messages )ι->void{
		for( const Proto::FromClientMessage& m : messages.messages() ){
			using enum Proto::FromClientMessage::ValueCase;
			var requestId = m.request_id();
			switch( m.Value_case() ){
			case kSessionId:
				OnConnect( m.session_id(), requestId );
			break;
			case kEcho:{
				Proto::FromServerTransmission t;
				auto res = t.add_messages();
				res->set_request_id( requestId );
				res->set_echo_text( m.echo() );
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