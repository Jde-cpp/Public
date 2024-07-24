#include <jde/web/server/IWebsocketSession.h>
#include <jde/web/server/Streams.h>

#define var const auto
namespace Jde::Web{
	auto _readTag{ Logging::Tag(ELogTags::SocketServerRead) };
	auto _writeTag{ Logging::Tag(ELogTags::SocketServerWrite) };
	α Server::SocketServerReadTag()ι->sp<Jde::LogTag>{ return _readTag; }
	α Server::SocketServerWriteTag()ι->sp<Jde::LogTag>{ return _writeTag; }
}
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
//		net::dispatch( Stream.GetExecutor(), beast::bind_front_handler(&IWebsocketSession::OnRun, shared_from_this()) );
	}

#define CHECK_EC(ec,tag,  ...) if( ec ){ CodeException x(static_cast<std::error_code>(ec), tag __VA_OPT__(,) __VA_ARGS__); return; }
	α IWebsocketSession::OnAccept( beast::error_code ec )ι->void{
		LogRead( "OnAccept", 0 );
		CHECK_EC( ec, _readTag );
		SendAck( Id() );
		DoRead();
	}

	α IWebsocketSession::DoRead()ι->void{
		Stream->DoRead( shared_from_this() );
	}

	α IWebsocketSession::OnWrite( beast::error_code ec, uint c )ι->void{
		boost::ignore_unused( c );
		try{
			THROW_IFX( ec, CodeException(static_cast<std::error_code>(ec), _writeTag, ec == websocket::error::closed ? ELogLevel::Trace : ELogLevel::Error) );
		}
		catch( const CodeException& )
		{}
	}

	α IWebsocketSession::LogRead( string&& what, RequestId requestId, ELogLevel level, SL sl )ι->void{//TODO forward args.
		Logging::Log( Logging::Message(level, "[{:x}.{:x}]{}", sl), SocketServerReadTag(), Id(), requestId, move(what) );
	}

	α IWebsocketSession::LogWrite( string&& what, RequestId requestId, ELogLevel level, SL sl )ι->void{
		Logging::Log( Logging::Message(level, "[{:x}.{:x}]{}", sl), SocketServerWriteTag(), Id(), requestId, move(what) );
	}
	α IWebsocketSession::LogWriteException( const IException& e, RequestId requestId, ELogLevel level, SL sl )ι->void{
		e.SetLevel( ELogLevel::NoLog );
		auto e2 = Exception{ sl, level, "[{}.{}]{}", 𐢜("{:x}", Id()), 𐢜("{:x}", requestId), e.what() };
	}

	α IWebsocketSession::OnClose()ι->void{
		LogRead( "OnClose.", 0 );
	}
}