#include "WebsocketMock.h"
#include <jde/web/flex/IWebsocketSession.h>
#include "ServerMock.h"

#define var const auto
namespace Jde::Web::Mock{
	WebsocketSession::WebsocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		base{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }
	{}

	α WebsocketSession::OnConnect( SessionPK sessionId, Http::RequestId requestId )ι->Web::UpsertAwait::Task{
		try{
			Proto::FromServerTransmission t;
			auto m = t.add_messages();
			auto info = co_await Web::UpsertAwait{ Jde::format("{:x}", sessionId), _userEndpoint.address().to_string(), true };
			auto ack = m->mutable_ack();
			ack->set_session_id( info.SessionId );
			ack->set_server_socket_id( Id() );
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
	α RequestHandler::RunWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->void{
		auto pSession = ms<WebsocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		pSession->Run();
	};

}