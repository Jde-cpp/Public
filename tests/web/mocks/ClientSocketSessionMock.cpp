#include "ClientSocketSessionMock.h"
#include <boost/unordered/concurrent_flat_map.hpp>
#include <jde/http/usings.h>
#include "ClientSocketAwait.h"

#define var const auto

namespace Jde::Web::Mock{
	static sp<LogTag> _incomingTag{ Logging::Tag( "client.socket.incoming" ) };
	static sp<LogTag> _outgoingTag{ Logging::Tag( "client.socket.outgoing" ) };
	//α Http::IncomingTag()ι->sp<LogTag>{ return _incomingTag; }

	//using namespace Jde::Web::Flex;

	α ClientSocketSession::OnAck( Proto::FromServer::Ack&& connect )ι{
		std::any hAny = IClientSocketSession::GetTask( connect.request_id() );
		auto h = std::any_cast<ClientSocketAwait<Proto::FromServer::Ack>::Handle>( &hAny );
		if( h ){
			h->promise().Result = move( connect );
			h->resume();//TODO 1 function
		}
		else
			CRITICALT( Http::IncomingTag(), "RequestId '{}' not found.", connect.request_id() );
	}


	ClientSocketSession::ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		base{ ioc, ctx }
	{}

/*	α ClientSocketSession::AddTask( RequestId requestId, HClientSocketMessageTask h )ι->void{
		_tasks.emplace( requestId, h );
	}
*/
	α ClientSocketSession::HandleException( std::any&& h, string&& what )ι{
		if( auto pEcho = std::any_cast<ClientSocketAwait<Proto::Echo>::Handle>( &h ) ){
			pEcho->promise().Exception = mu<Exception>( what );
			pEcho->resume();
		}
		else if( auto pAck = std::any_cast<ClientSocketAwait<Proto::FromServer::Ack>::Handle>( &h ) ){
			pAck->promise().Exception = mu<Exception>( what );
			pAck->resume();
		}
		else{
			WARNT( _incomingTag, "Failed to process incomming exception '{}'.", what );
		}
	}

	α ClientSocketSession::OnRead( Http::Proto::TestFromServer&& transmission )ι->void{
		auto size = transmission.messages_size();
		for( auto i=0; i<size; ++i ){
		//for( auto&& m : transmission.mutable_messages() ){
			auto m = transmission.mutable_messages( i );
			using enum Proto::TestFromServerUnion::ValueCase;
			switch( m->Value_case() ){
			case kAck:
				HandleAck( m->ack() );
				OnAck( move(*m->mutable_ack()) );
				INFOT( IncomingTag(), "[{:x}]ClientSocketSession Created.", m->ack().session_id() );
				break;
			case kEcho:{
				auto h = std::any_cast<ClientSocketAwait<Proto::Echo>::Handle>( IClientSocketSession::GetTask(m->echo().request_id()) );
				h.promise().Result = move( *m->mutable_echo() );
				h.resume();
				}break;
			case kException:{
				auto& e = *m->mutable_exception();
				var requestId = e.request_id();
				std::any h = requestId==0 ? coroutine_handle<>{} : GetTask( e.request_id() );
				HandleException( move(h), move(*e.mutable_message()) );
			}break;
			default:
				BREAK;
			}
		}
	}
	α ClientSocketSession::Connect( SessionPK sessionId, SL sl )ι->ClientSocketAwait<Proto::FromServer::Ack>{
		Proto::TestFromClientUnion request;
		request.mutable_connect()->set_session_id( sessionId );
		var requestId = NextRequestId();
		request.mutable_connect()->set_request_id( requestId );
		Proto::TestFromClient requests;
		requests.add_messages()->CopyFrom( request );
		return ClientSocketAwait<Proto::FromServer::Ack>{ shared_from_this(), requestId, IO::Proto::ToString(requests), sl };
	}
	α ClientSocketSession::Echo( str x, SL sl )ι->ClientSocketAwait<Proto::Echo>{
		Proto::TestFromClientUnion request;
		request.mutable_echo()->set_echo_text( x );
		var requestId = NextRequestId();
		request.mutable_echo()->set_request_id( requestId );
		Proto::TestFromClient requests;
		requests.add_messages()->CopyFrom( request );
		return ClientSocketAwait<Proto::Echo>{ shared_from_this(), requestId, IO::Proto::ToString(requests), sl };
	}
	α ClientSocketSession::CloseServerSide( SL sl )ι->ClientSocketAwait<Proto::Echo>{
		Proto::TestFromClientUnion request;
		request.mutable_close_server_side();
		Proto::TestFromClient t;
		t.add_messages()->CopyFrom( request );
		var requestId = NextRequestId();
		return ClientSocketAwait<Proto::Echo>{ shared_from_this(), requestId, IO::Proto::ToString(t), sl };
	}
	α ClientSocketSession::BadTransmissionClient( SL sl )ι->ClientSocketAwait<Proto::Echo>{
		var requestId =  NextRequestId();
		return ClientSocketAwait<Proto::Echo>{ shared_from_this(), requestId, "ABCDEFG", sl }; //need destructed.
	}
	α ClientSocketSession::BadTransmissionServer( SL sl )ι->ClientSocketAwait<Proto::Echo>{
		Proto::TestFromClientUnion request;
		request.mutable_bad_transmission_server();
		Proto::TestFromClient t;
		t.add_messages()->CopyFrom( move(request) );
		var requestId = NextRequestId();
		return ClientSocketAwait<Proto::Echo>{ shared_from_this(), requestId, IO::Proto::ToString(t), sl };
	}

	α ClientSocketSession::OnClose( beast::error_code ec )ι->void{
		auto f = [this, ec](std::any&& h)->void { HandleException(move(h), CodeException{ec}.what() ); };
		CloseTasks( f );
		base::OnClose( ec );
	}
}