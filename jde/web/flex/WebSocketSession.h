#pragma once
#include "Streams.h"

namespace Jde::Web::Flex{
	namespace beast = boost::beast;
	namespace net = boost::asio;
	namespace websocket = beast::websocket;
	namespace http = beast::http;

	α Fail( beast::error_code ec, char const* what )ι->void;
	//share ssl and non-ssl
	//template<class TDerived>
	struct TWebSocketSession /*abstract*/{
    α Run( TRequestType req ){ DoAccept( move(req) ); }
	private:
    //α Derived()ι->TDerived&{ return static_cast<TDerived&>(*this); }
    α DoAccept( TRequestType req )ι->void;
    α OnAccept(beast::error_code ec)->void{ if(ec) return Fail( ec, "accept" ); DoRead(); }
  //  α DoRead()->void{ Derived().Stream().async_read( _buffer, beast::bind_front_handler( &TWebSocketSession::OnRead, Derived().shared_from_this()) ); }
		β DoRead()->void=0;
		α OnRead( beast::error_code ec, uint bytes_transferred )ι->void;
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;

	  beast::flat_buffer _buffer;
	};
	//non-ssl
	struct IWebsocketSession final: TWebSocketSession, std::enable_shared_from_this<IWebsocketSession>{
		explicit IWebsocketSession( beast::tcp_stream&& stream ) : _ws(std::move(stream)){}
		α Stream()ι->websocket::stream<beast::tcp_stream>&{	return _ws; }
	private:
		websocket::stream<beast::tcp_stream> _ws;
	};

	//ssl
	struct ISslWebsocketSession final: TWebSocketSession, std::enable_shared_from_this<ISslWebsocketSession>{
			explicit ISslWebsocketSession( beast::ssl_stream<beast::tcp_stream>&& stream ): _ws( std::move(stream) ){}
			α Stream()ι->beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>&{ return _ws; }
		private:
			websocket::stream<beast::ssl_stream<beast::tcp_stream>> _ws;
	};

	template<typename TStream/*, typename TBody, typename TAllocator*/>
	[[nodiscard]] α RunWebsocketSession( TStream& stream, beast::flat_buffer& buffer, TRequestType req )->net::awaitable<void, executor_type>{
    beast::websocket::stream<TStream&> ws{stream};
    ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );// Set suggested timeout settings for the websocket
    ws.set_option( websocket::stream_base::decorator( [](websocket::response_type& res){// Set a decorator to change the Server of the handshake
			res.set( http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " advanced-server-flex" ); 
		}) );
    
    auto [ec] = co_await ws.async_accept(req); // Accept the websocket handshake
    if (ec)
      co_return Fail(ec, "accept");

    while( true ){
			uint bytes_transferred = 0u;
			std::tie( ec, bytes_transferred ) = co_await ws.async_read( buffer );
			if( ec == websocket::error::closed )
				co_return;
			if( ec )
				co_return Fail(ec, "read");

			ws.text( ws.got_text() );
			std::tie( ec, bytes_transferred ) = co_await ws.async_write( buffer.data() );
			if (ec)
				co_return Fail(ec, "write");
			buffer.consume(buffer.size());
    }
	}
/*
	//template<class TBody, class TAllocator>
	α MakeWebsocketSession( beast::tcp_stream stream, TRequestType req )->void{
    ms<IWebsocketSession>( std::move(stream) )->Run( std::move(req) );
	}
	
	//template<class TBody, class TAllocator>
	α MakeWebsocketSession( beast::ssl_stream<beast::tcp_stream> stream, TRequestType req )->void{
    ms<ISslWebsocketSession>( std::move(stream) )->Run( std::move(req) );
	}
*/	
/*
	template<class TDerived>
	template<class TBody, class TAllocator>
	α TWebSocketSession<TDerived>::DoAccept( TRequestType req )ι->void{
		Derived().Stream().set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );// Set suggested timeout settings for the websocket
		Derived().Stream().set_option( websocket::stream_base::decorator( [](websocket::response_type& res){// Set a decorator to change the Server of the handshake
			res.set( http::field::server, string(BOOST_BEAST_VERSION_STRING) + " advanced-server-flex" );
		}));
		Derived().Stream().async_accept( req, beast::bind_front_handler(&TWebSocketSession::OnAccept, Derived().shared_from_this()) );
	}
*/	
}

