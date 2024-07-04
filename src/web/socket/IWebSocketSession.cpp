#include <jde/web/socket/IWebSocketSession.h>
#include <jde/Exception.h>

#define var const auto
namespace Jde::Web::Socket{
	sp<Jde::LogTag> _logTag = Logging::Tag( "webSocket.Requests" );
	α WebSocketRequestTag()ι->sp<Jde::LogTag>{ return _logTag; }

	α IWebSocketSession::Run()ι->void{
		TRACE( "[{}]Socket::Run()", Id );
		net::dispatch( _ws.get_executor(), beast::bind_front_handler(&IWebSocketSession::OnRun, shared_from_this()) );
	}

	α IWebSocketSession::OnRun()ι->void{
		TRACE( "[{}]Socket::OnRun()", Id );
		_ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );
		_ws.set_option( websocket::stream_base::decorator([]( websocket::response_type& res )
			{
				res.set( http::field::server, string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async" );
			}) );
		_ws.async_accept( beast::bind_front_handler(&IWebSocketSession::OnAccept,shared_from_this()) );
	}
#define CHECK_EC(ec, ...) if( ec ){ CodeException x(static_cast<std::error_code>(ec) __VA_OPT__(,) __VA_ARGS__); return; }
	α IWebSocketSession::OnAccept( beast::error_code ec )ι->void{
		TRACE( "[{}]Socket::OnAccept()", Id );
		CHECK_EC( ec );
		DoRead();
	}

	std::chrono::steady_clock::time_point readProcessingTime = std::chrono::steady_clock::now();
	α IWebSocketSession::DoRead()ι->void{
		TRACE( "[{}]Socket::DoRead(readProcessingTime={})", Id, Chrono::ToString(readProcessingTime-std::chrono::steady_clock::now()) );
		_ws.async_read( _buffer, [p=shared_from_this()]( beast::error_code ec, uint c )ι{
			readProcessingTime = std::chrono::steady_clock::now();
			boost::ignore_unused( c );
			if( ec ){
				using namespace boost::asio::error;
				bool aborted = ec == connection_aborted;
				var level = ec == websocket::error::closed || aborted || ec==not_connected || ec==connection_reset ? ELogLevel::Trace : ELogLevel::Error;
				p->Disconnect( CodeException{static_cast<std::error_code>(ec), level} );
				return;
			}
			TRACE( "[{}]Socket::DoRead({})", p->Id, c );
			p->OnRead( (char*)p->_buffer.data().data(), p->_buffer.size() );
			p->_buffer.clear();
			p->DoRead();
		} );
	}

	α IWebSocketSession::OnWrite( beast::error_code ec, uint c )ι->void{
		boost::ignore_unused( c );
		try{
			THROW_IFX( ec, CodeException(static_cast<std::error_code>(ec), ec == websocket::error::closed ? ELogLevel::Trace : ELogLevel::Error) );
		}
		catch( const CodeException& )
		{}
	}
}