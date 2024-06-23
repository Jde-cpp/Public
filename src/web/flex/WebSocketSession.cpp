#include <jde/web/flex/WebSocketSession.h>

namespace Jde::Web{
namespace Flex{
	α TWebSocketSession::OnRead( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused(bytes_transferred);
		if(ec == websocket::error::closed)
			return;
		if(ec)
			return Fail(ec, "read");
		// Echo the message
		//Derived().Stream().text( Derived().Stream().got_text() );
		//  Derived().Stream().async_write( _buffer.data(), beast::bind_front_handler(&TWebSocketSession::on_write, Derived().shared_from_this()) );
	}
	α TWebSocketSession::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused(bytes_transferred);
		if(ec)
			return Fail(ec, "write");
		_buffer.consume( _buffer.size() );
		DoRead();
	}
}}