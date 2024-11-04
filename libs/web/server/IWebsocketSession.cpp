#include <jde/web/server/IWebsocketSession.h>
#include "Streams.h"

#define let const auto

namespace Jde::Web::Server{
	IWebsocketSession::IWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι:
		Stream{ ms<SocketStream>(move(stream), move(buffer)) },
		_userEndpoint{ userEndpoint },
		_initialRequest{ move(request) },
		_id{ connectionIndex }
	{}

	α IWebsocketSession::Run()ι->void{
		LogRead( "Run", 0 );
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
		Log( level, ELogTags::SocketServerRead, sl, "[{:x}.{:x}]{}", Id(), requestId, move(what) );
	}

	α IWebsocketSession::LogWrite( string&& what, RequestId requestId, ELogLevel level, SL sl )ι->void{
		Log( level, ELogTags::SocketServerWrite, sl, "[{:x}.{:x}]{}", Id(), requestId, move(what) );
	}

	α IWebsocketSession::LogWriteException( const IException& e, RequestId requestId, ELogLevel level, SL sl )ι->void{
		e.SetLevel( ELogLevel::NoLog );
		Exception{ sl, level, "[{}.{}]{}", Ƒ("{:x}", Id()), Ƒ("{:x}", requestId), e.what() }; //:x doesn't work with exception formatter
	}

	α IWebsocketSession::Close()ι->void{ Stream->Close( shared_from_this() ); }
	α IWebsocketSession::OnClose()ι->void{
		LogRead( "OnClose.", 0 );
	}
}