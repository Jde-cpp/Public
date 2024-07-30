#include "ServerSocketSession.h"
#include <jde/web/server/IWebsocketSession.h>
#include "ServerMock.h"

#define var const auto
namespace Jde::Web::Mock{
	ServerSocketSession::ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		base{ move(stream), move(buffer), move(request), move(userEndpoint), connectionIndex }
	{}

	α ServerSocketSession::OnConnect( SessionPK sessionId, RequestId requestId )ι->Server::Sessions::UpsertAwait::Task{
			try{
			auto info = co_await Server::Sessions::UpsertAwait{ 𐢜("{:x}", sessionId), _userEndpoint.address().to_string(), true };
			Proto::FromServerTransmission t;
			auto m = t.add_messages();
			m->set_request_id( requestId );
			m->set_session_id( sessionId );
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

	α ServerSocketSession::WriteException( IException&& e )ι->void{
		Proto::FromServerTransmission t;
		auto m = t.add_messages();
		m->set_exception( e.what() );
		Write( move(t) );
	}

	α ServerSocketSession::SendAck( uint id )ι->void{
		LogWrite( 𐢜("Ack id: {:x}", id), 0 );
		Proto::FromServerTransmission t;
		t.add_messages()->set_ack( id );
		Write( move(t) );
	}


	α ServerSocketSession::OnRead( Proto::FromClientTransmission&& messages )ι->void{
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
		auto pSession = ms<ServerSocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		pSession->Run();
	};

}