#include <jde/web/flex/IWebsocketSession.h>
#include <jde/web/flex/Streams.h>
#define var const auto
namespace Jde::Web::Flex{
	sp<Jde::LogTag> _logTag = Logging::Tag( "webSocket.Requests" );//TODO
	α WebsocketRequestTag()ι->sp<Jde::LogTag>{ return _logTag; }
	atomic<uint> _sequence = 0;

	IWebsocketSession::IWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType request, tcp::endpoint&& userEndpoint )ι:
		Stream{ ms<SocketStream>(move(stream), move(buffer)) },
		_userEndpoint{ userEndpoint },
		_initialRequest{ move(request) },
		_id{ ++_sequence }
	{}

	α IWebsocketSession::Run()ι->void{
		TRACE( "[{}]Socket::Run()", _id );
		Stream->DoAccept( move(_initialRequest), shared_from_this() );
//		net::dispatch( Stream.GetExecutor(), beast::bind_front_handler(&IWebsocketSession::OnRun, shared_from_this()) );
	}

	// α IWebsocketSession::OnRun()ι->void{
	// 	TRACE( "[{}]Socket::OnRun()", _id );
	// 	Stream.OnRun( shared_from_this() );
	// }
#define CHECK_EC(ec,tag,  ...) if( ec ){ CodeException x(static_cast<std::error_code>(ec), tag __VA_OPT__(,) __VA_ARGS__); return; }
	α IWebsocketSession::OnAccept( beast::error_code ec )ι->void{
		TRACE( "[{}]Socket::OnAccept()", _id );
		CHECK_EC( ec, _logTag );
		DoRead();
	}

	α IWebsocketSession::DoRead()ι->void{
		Stream->DoRead( shared_from_this() );
	}

	α IWebsocketSession::OnWrite( beast::error_code ec, uint c )ι->void{
		boost::ignore_unused( c );
		try{
			THROW_IFX( ec, CodeException(static_cast<std::error_code>(ec), _logTag, ec == websocket::error::closed ? ELogLevel::Trace : ELogLevel::Error) );
		}
		catch( const CodeException& )
		{}
	}
}