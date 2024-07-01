#pragma once
#include "Streams.h"

namespace Jde::Web::Flex{
	namespace beast = boost::beast;
	namespace net = boost::asio;
	namespace websocket = beast::websocket;
	namespace http = beast::http;
/*
	α Fail( beast::error_code ec, char const* what )ι->void;
	//share ssl and non-ssl

	template<class TStream>
	struct TWebsocketSession /*abstract* /{
    α Run( TRequestType req ){ DoAccept( move(req) ); }
	private:
    α Derived()ι->TStream&{ return static_cast<TStream&>(*this); }
    α DoAccept( TRequestType req )ι->void;
    α OnAccept( beast::error_code ec )->void{ if(ec) return Fail( ec, "accept" ); DoRead(); }
    α DoRead()->void{ Derived().Stream().async_read( _buffer, beast::bind_front_handler( &TWebsocketSession::OnRead, Derived().shared_from_this()) ); }
		α OnRead( beast::error_code ec, uint bytes_transferred )ι->void;
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;

	  beast::flat_buffer _buffer;
	};
	//non-ssl
	struct IPlainWebsocketSession final: TWebsocketSession<IPlainWebsocketSession>, std::enable_shared_from_this<IPlainWebsocketSession>{
		explicit IPlainWebsocketSession( beast::tcp_stream&& stream ) : _ws(std::move(stream)){}
		α Stream()ι->websocket::stream<beast::tcp_stream>&{	return _ws; }
	private:
		websocket::stream<beast::tcp_stream> _ws;
	};

	//ssl
	struct ISslWebsocketSession final: TWebsocketSession<ISslWebsocketSession>, std::enable_shared_from_this<ISslWebsocketSession>{
		explicit ISslWebsocketSession( beast::ssl_stream<beast::tcp_stream>&& stream ): _ws( std::move(stream) ){}
		α Stream()ι->beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>&{ return _ws; }
	private:
		websocket::stream<beast::ssl_stream<beast::tcp_stream>> _ws;
	};


	Ξ MakeWebsocketSession( beast::tcp_stream stream, TRequestType req )->void{
    ms<IPlainWebsocketSession>( std::move(stream) )->Run( std::move(req) );
	}

	Ξ MakeWebsocketSession( beast::ssl_stream<beast::tcp_stream> stream, TRequestType req )->void{
    ms<ISslWebsocketSession>( std::move(stream) )->Run( std::move(req) );
	}

	template<class TStream>
	α TWebsocketSession<TStream>::DoAccept( TRequestType req )ι->void{
		Derived().Stream().set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );// Set suggested timeout settings for the websocket
		Derived().Stream().set_option( websocket::stream_base::decorator( [](websocket::response_type& res){// Set a decorator to change the Server of the handshake
			res.set( http::field::server, string(BOOST_BEAST_VERSION_STRING) + " advanced-server-flex" );
		}));
		Derived().Stream().async_accept( req, beast::bind_front_handler(&TWebsocketSession::OnAccept, Derived().shared_from_this()) );
	}

	Ŧ TWebsocketSession<T>::OnRead( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused(bytes_transferred);
		if(ec == websocket::error::closed)
			return;
		if(ec)
			return Fail(ec, "read");
		// Echo the message
		//Derived().Stream().text( Derived().Stream().got_text() );
		//  Derived().Stream().async_write( _buffer.data(), beast::bind_front_handler(&TWebsocketSession::on_write, Derived().shared_from_this()) );
	}
	Ŧ TWebsocketSession<T>::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused(bytes_transferred);
		if(ec)
			return Fail(ec, "write");
		_buffer.consume( _buffer.size() );
		DoRead();
	}

/*	template<typename TStream/  *, typename TBody, typename TAllocator>
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
*/

}

