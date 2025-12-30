#include "ClientSocketSession.h"
#include <jde/web/usings.h>
#include <jde/app/proto/app.FromServer.h>
#include "ServerMock.h"

#define let const auto

namespace Jde::Web::Mock{
	ClientSocketSession::ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		base{ ioc, ctx }
	{}

	α ClientSocketSession::OnAck( uint32 serverSocketId )ι->void{
		SetId( serverSocketId );
		INFOT( ELogTags::SocketClientRead, "[{}] {} AppClientSocketSession created: {}.", Id(), IsSsl() ? "Ssl" : "Plain", Host() );
	}

	α ClientSocketSession::SetSessionId( str strSessionId, RequestId /*requestId*/ )->Server::Sessions::UpsertAwait::Task{
		//LogRead( Ƒ("sessionId: '{}'", strSessionId), requestId );
		try{
			auto sessionInfo = co_await Server::Sessions::UpsertAwait( strSessionId, "127.0.0.1", true, AppClient() );
			auto t = App::FromServer::Session( move(*sessionInfo), 0 );
			base::SetInfo( move(*t.mutable_messages(0)->mutable_session_info()) );
			//Write( FromServer::CompleteTrans(requestId) );
		}
		catch( IException& e ){
			//WriteException( move(e), requestId );
		}
	}


	α ClientSocketSession::HandleException( std::any&& h, string&& what )ι{
		if( auto pEcho = std::any_cast<ClientSocketAwait<string>::Handle>(&h) ){
			pEcho->promise().SetExp( Exception{what} );
			pEcho->resume();
		}
		else if( auto pAck = std::any_cast<ClientSocketAwait<SessionPK>::Handle>(&h) ){
			pAck->promise().SetExp( Exception{what} );
			pAck->resume();
		}
		else{
			WARNT( ELogTags::SocketClientRead, "Failed to process incomming exception '{}'.", what );
		}
	}

	α ClientSocketSession::OnRead( Proto::FromServerTransmission&& transmission )ι->void{
		auto size = transmission.messages_size();
		for( auto i=0; i<size; ++i ){
			auto m = transmission.mutable_messages( i );
			using enum Proto::FromServerMessage::ValueCase;
			let requestId = m->request_id();
			switch( m->Value_case() ){
			case kAck:
				OnAck( m->ack() );
				break;
			case kSessionId:{
				auto h = std::any_cast<ClientSocketAwait<SessionPK>::Handle>( IClientSocketSession::PopTask(requestId) );
				SetSessionId(Ƒ("{:x}", m->session_id()), requestId );
				h.promise().Resume( m->session_id(), h );
				break;}
			case kEchoText:{
				auto h = std::any_cast<ClientSocketAwait<string>::Handle>( IClientSocketSession::PopTask(requestId) );
				h.promise().SetValue( move(*m->mutable_echo_text()) );
				h.resume();
				break;}
			case kException:{
				std::any h = requestId==0 ? coroutine_handle<>{} : PopTask( requestId );
				HandleException( move(h), move(*m->mutable_exception()) );
				break;}
			default:
				BREAK;
			}
		}
	}
	α ClientSocketSession::Connect( SessionPK sessionId, SL sl )ι->ClientSocketAwait<SessionPK>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->set_session_id( sessionId );
		let requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<SessionPK>{ Protobuf::ToString(t), requestId, shared_from_this(), sl };
	}
	α ClientSocketSession::Echo( str x, SL sl )ι->ClientSocketAwait<string>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->set_echo( x );
		let requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<string>{ Protobuf::ToString(t), requestId, shared_from_this(), sl };
	}
	α ClientSocketSession::CloseServerSide( SL sl )ι->ClientSocketAwait<string>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->mutable_close_server_side();
		let requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<string>{ Protobuf::ToString(t), requestId, shared_from_this(), sl };
	}
	α ClientSocketSession::BadTransmissionClient( SL sl )ι->ClientSocketAwait<string>{
		let requestId =  NextRequestId();
		return ClientSocketAwait<string>{ "ABCDEFG", requestId, shared_from_this(), sl }; //need destructed.
	}
	α ClientSocketSession::BadTransmissionServer( SL sl )ι->ClientSocketAwait<string>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->mutable_bad_transmission_server();
		let requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<string>{ Protobuf::ToString(t), requestId, shared_from_this(), sl };
	}

	α ClientSocketSession::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec]( std::any&& h )->void { HandleException(move(h), CodeException{ec, ELogTags::SocketClientWrite, ELogLevel::NoLog}.what()); };
		CloseTasks( f );
		base::OnClose( ec );
	}
}