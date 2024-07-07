#include "ClientSocketSessionMock.h"
#include <boost/unordered/concurrent_flat_map.hpp>
#include <jde/http/usings.h>

#define var const auto

namespace Jde::Web::Mock{
	static sp<LogTag> _incomingTag{ Logging::Tag( "client.socket.incoming" ) };
	static sp<LogTag> _outgoingTag{ Logging::Tag( "client.socket.outgoing" ) };
	ClientSocketSession::ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		base{ ioc, ctx }
	{}

	α ClientSocketSession::OnAck( RequestId requestId, SessionPK sessionId )ι->void{
		std::any hAny = IClientSocketSession::GetTask( requestId );
		auto h = std::any_cast<ClientSocketAwait<SessionPK>::Handle>( &hAny );
		if( h ){
			h->promise().Result = move( sessionId );
			h->resume();//TODO 1 function
		}
		else
			CRITICALT( Http::IncomingTag(), "RequestId '{}' not found.", requestId );
	}


	α ClientSocketSession::HandleException( std::any&& h, string&& what )ι{
		if( auto pEcho = std::any_cast<ClientSocketAwait<string>::Handle>( &h ) ){
			pEcho->promise().Exception = mu<Exception>( what );
			pEcho->resume();
		}
		else if( auto pAck = std::any_cast<ClientSocketAwait<SessionPK>::Handle>( &h ) ){
			pAck->promise().Exception = mu<Exception>( what );
			pAck->resume();
		}
		else{
			WARNT( _incomingTag, "Failed to process incomming exception '{}'.", what );
		}
	}

	α ClientSocketSession::OnRead( Http::Proto::FromServerTransmission&& transmission )ι->void{
		auto size = transmission.messages_size();
		for( auto i=0; i<size; ++i ){
		//for( auto&& m : transmission.mutable_messages() ){
			auto m = transmission.mutable_messages( i );
			using enum Proto::FromServerMessage::ValueCase;
			var requestId = m->request_id();
			switch( m->Value_case() ){
			case kAck:
				SetSessionId( m->ack() );
				OnAck( requestId, m->ack() );
				INFOT( IncomingTag(), "[{:x}]ClientSocketSession Created.", m->ack() );
				break;
			case kEchoText:{
				auto h = std::any_cast<ClientSocketAwait<string>::Handle>( IClientSocketSession::GetTask(requestId) );
				h.promise().Result = move( *m->mutable_echo_text() );
				h.resume();
				}break;
			case kException:{
				std::any h = requestId==0 ? coroutine_handle<>{} : GetTask( requestId );
				HandleException( move(h), move(*m->mutable_exception()) );
			}break;
			default:
				BREAK;
			}
		}
	}
	α ClientSocketSession::Connect( SessionPK sessionId, SL sl )ι->ClientSocketAwait<SessionPK>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->set_session_id( sessionId );
		var requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<SessionPK>{ shared_from_this(), requestId, IO::Proto::ToString(t), sl };
	}
	α ClientSocketSession::Echo( str x, SL sl )ι->ClientSocketAwait<string>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->set_echo( x );
		var requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<string>{ shared_from_this(), requestId, IO::Proto::ToString(t), sl };
	}
	α ClientSocketSession::CloseServerSide( SL sl )ι->ClientSocketAwait<string>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->mutable_close_server_side();
		var requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<string>{ shared_from_this(), requestId, IO::Proto::ToString(t), sl };
	}
	α ClientSocketSession::BadTransmissionClient( SL sl )ι->ClientSocketAwait<string>{
		var requestId =  NextRequestId();
		return ClientSocketAwait<string>{ shared_from_this(), requestId, "ABCDEFG", sl }; //need destructed.
	}
	α ClientSocketSession::BadTransmissionServer( SL sl )ι->ClientSocketAwait<string>{
		Proto::FromClientTransmission t;
		auto request = t.add_messages();
		request->mutable_bad_transmission_server();
		var requestId = NextRequestId();
		request->set_request_id( requestId );
		return ClientSocketAwait<string>{ shared_from_this(), requestId, IO::Proto::ToString(t), sl };
	}

	α ClientSocketSession::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec](std::any&& h)->void { HandleException(move(h), CodeException{ec, _outgoingTag}.what() ); };
		CloseTasks( f );
		base::OnClose( ec );
	}
}