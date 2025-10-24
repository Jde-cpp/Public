#include <jde/web/server/IWebsocketSession.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalSubscriptions.h>
#include "ServerImpl.h"

#define let const auto

namespace Jde::Web::Server{
	struct SocketServerListener : QL::IListener{
		SocketServerListener( sp<IWebsocketSession> session )ι: QL::IListener{Ƒ("[{}]Socket", session->Id())}, _session{ session }{}
		α OnChange( const jvalue& j, QL::SubscriptionId clientId )ε->void{ _session->WriteSubscription( j, clientId ); }
		sp<IWebsocketSession> _session;
	};

	IWebsocketSession::IWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		Stream{ ms<SocketStream>(move(stream), move(buffer)) },
		_userEndpoint{ userEndpoint },
		_initialRequest{ move(request) },
		_id{ connectionIndex }
	{}

	α IWebsocketSession::Run()ι->void{
		LogRead( "Run", 0 );
		SocketServerListener foo{ shared_from_this() };
		_listener = ms<SocketServerListener>( shared_from_this() );
		Stream->DoAccept( move(_initialRequest), shared_from_this() );
	}

#define CHECK_EC(ec,tag,  ...) if( ec ){ CodeException(static_cast<std::error_code>(ec), tag __VA_OPT__(,) __VA_ARGS__); return; }
	α IWebsocketSession::OnAccept( beast::error_code ec )ι->void{
		LogRead( "OnAccept", 0 );
		CHECK_EC( ec, ELogTags::SocketServerRead );
		SendAck( Id() );
		DoRead();
	}

	α IWebsocketSession::DoRead()ι->void{
		Stream->DoRead( shared_from_this() );
	}

	α IWebsocketSession::Write( string&& m )ι->void{
		Stream->Write( move(m) );
	}

	α IWebsocketSession::OnWrite( beast::error_code ec, uint c )ι->void{
		boost::ignore_unused( c );
		try{
			THROW_IFX( ec, CodeException(static_cast<std::error_code>(ec), ELogTags::SocketServerWrite, ec == websocket::error::closed ? ELogLevel::Trace : ELogLevel::Error) );
		}
		catch( const CodeException& )
		{}
	}

	α IWebsocketSession::LogRead( string&& what, RequestId requestId, ELogLevel level, SL sl )ι->void{//TODO forward args.
		Logging::Log( level, ELogTags::SocketServerRead, sl, "[{:x}.{:x}]{}", Id(), requestId, move(what) );
	}

	α IWebsocketSession::LogWrite( string&& what, RequestId requestId, ELogLevel level, SL sl )ι->void{
		Logging::Log( level, ELogTags::SocketServerWrite, sl, "[{:x}.{:x}]{}", Id(), requestId, move(what) );
	}

	α IWebsocketSession::LogWriteException( const exception& e, RequestId requestId, ELogLevel level, SL sl )ι->void{
		if( let p = dynamic_cast<const IException*>(&e); p )
			p->SetLevel( ELogLevel::NoLog );
		Exception{ sl, level, "[{}.{}]{}", Ƒ("{:x}", Id()), Ƒ("{:x}", requestId), e.what() }; //:x doesn't work with exception formatter
	}
	α IWebsocketSession::LogWriteException( str e, RequestId requestId, ELogLevel level, SL sl )ι->void{
		Exception{ sl, level, "[{}.{}]{}", Ƒ("{:x}", Id()), Ƒ("{:x}", requestId), move(e) }; //:x doesn't work with exception formatter
	}

	α IWebsocketSession::Close()ι->void{
		if( _listener ){
			QL::Subscriptions::StopListen( _listener );
			_listener = nullptr;
		}
		Stream->Close( shared_from_this() );
	}
	α IWebsocketSession::OnClose()ι->void{
		LogRead( "SererSocket::OnClose.", 0 );
		Internal::RemoveSocketSession( Id() );
		_listener = nullptr;
	}

	α IWebsocketSession::AddSubscription( string&& query, jobject variables, RequestId /*requestId*/, SL sl )ε->vector<QL::SubscriptionId>{
		auto subs = QL::ParseSubscriptions( move(query), variables, Schemas(), sl );
		vector<QL::SubscriptionId> subscriptionIds;
		for( auto& sub : subs )
			subscriptionIds.emplace_back( sub.Id );
		if( _listener )
			QL::Subscriptions::Listen( _listener, move(subs) );
		return subscriptionIds;
	}
	α IWebsocketSession::RemoveSubscription( vector<QL::SubscriptionId>&& ids, RequestId requestId, SL /*sl*/ )ι->void{
		try{
			QL::Subscriptions::StopListen( _listener, move(ids) );
		}
		catch( std::exception& e ){
			WriteException( move(e), requestId );
		}
	}
}