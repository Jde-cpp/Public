#include "ClientSocketSession.h"
#include <boost/unordered/concurrent_flat_map.hpp>
#include <jde/web/client/usings.h>

#define let const auto

namespace Jde::Web::Mock{
	ClientSocketSession::ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		base{ ioc, ctx }
	{}

	α ClientSocketSession::OnAck( uint32 serverSocketId )ι->void{
		SetId( serverSocketId );
		INFOT( ELogTags::SocketClientRead, "[{}] {} AppClientSocketSession created: {}.", Id(), IsSsl() ? "Ssl" : "Plain", Host() );
		//ResumeScaler<SessionPK>( move(hAny), SessionId() );
	}


	α ClientSocketSession::HandleException( std::any&& h, string&& what )ι{
		if( auto pEcho = std::any_cast<ClientSocketAwait<string>::Handle>( &h ) ){
			pEcho->promise().SetExp( Exception{what} );
			pEcho->resume();
		}
		else if( auto pAck = std::any_cast<ClientSocketAwait<SessionPK>::Handle>( &h ) ){
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
		//for( auto&& m : transmission.mutable_messages() ){
			auto m = transmission.mutable_messages( i );
			using enum Proto::FromServerMessage::ValueCase;
			let requestId = m->request_id();
			switch( m->Value_case() ){
			case kAck:
				OnAck( m->ack() );
				break;
			case kSessionId:{
				auto h = std::any_cast<ClientSocketAwait<SessionPK>::Handle>( IClientSocketSession::PopTask(requestId) );
				SetSessionId( m->session_id() );
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
		return ClientSocketAwait<SessionPK>{ Jde::Proto::ToString(t), requestId, shared_from_this(), sl };
	}
	α ClientSocketSession::Echo( str x, SL sl )ι->ClientSocketAwait<string>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->set_echo( x );
		let requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<string>{ Jde::Proto::ToString(t), requestId, shared_from_this(), sl };
	}
	α ClientSocketSession::CloseServerSide( SL sl )ι->ClientSocketAwait<string>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->mutable_close_server_side();
		let requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<string>{ Jde::Proto::ToString(t), requestId, shared_from_this(), sl };
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
		return ClientSocketAwait<string>{ Jde::Proto::ToString(t), requestId, shared_from_this(), sl };
	}

	α ClientSocketSession::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec](std::any&& h)->void { HandleException(move(h), CodeException{ec, ELogTags::SocketClientWrite, ELogLevel::NoLog}.what()); };
		CloseTasks( f );
		base::OnClose( ec );
	}
}