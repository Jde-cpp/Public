#pragma once

namespace Jde::Web::Flex{
	namespace beast = boost::beast;
	namespace net = boost::asio;
	namespace websocket = beast::websocket;
	namespace http = beast::http;
	
	using executor_type = net::io_context::executor_type;	

	α Fail( beast::error_code ec, char const* what )->void;
	//share ssl and non-ssl
	template<class TDerived>
	struct TWebSocketSession{
    template<class TBody, class TAllocator>
    α Run( http::request<TBody, http::basic_fields<TAllocator>> req ){ DoAccept( move(req) ); }

	private:
    α Derived()ι->TDerived&{ return static_cast<TDerived&>(*this); }
    template<class TBody, class TAllocator>
    α DoAccept(http::request<TBody, http::basic_fields<TAllocator>> req)ι->void;

    α OnAccept(beast::error_code ec)->void{
			if(ec) return Fail( ec, "accept" );
      DoRead();
    }
    α DoRead()->void{ Derived().Stream().async_read( _buffer, beast::bind_front_handler( &TWebSocketSession::OnRead, Derived().shared_from_this()) ); }
    
		α OnRead( beast::error_code ec, uint bytes_transferred )ι->void{
			boost::ignore_unused(bytes_transferred);
			if(ec == websocket::error::closed)
				return;
      if(ec)
        return Fail(ec, "read");
      // Echo the message
      Derived().Stream().text( Derived().Stream().got_text() );
        Derived().Stream().async_write( _buffer.data(), beast::bind_front_handler(&TWebSocketSession::on_write, Derived().shared_from_this()) );
    }
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
      boost::ignore_unused(bytes_transferred);
      if(ec)
        return Fail(ec, "write");
      _buffer.consume( _buffer.size() );
      DoRead();
    }

	  beast::flat_buffer _buffer;
	};
	//non-ssl
	struct IWebsocketSession : TWebSocketSession<IWebsocketSession>, std::enable_shared_from_this<IWebsocketSession>{
		explicit IWebsocketSession( beast::tcp_stream&& stream ) : _ws(std::move(stream)){}
		α Stream()ι->websocket::stream<beast::tcp_stream>&{	return _ws; }
	private:
		websocket::stream<beast::tcp_stream> _ws;
	};

	//ssl
	struct ISslWebsocketSession : TWebSocketSession<ISslWebsocketSession>, std::enable_shared_from_this<ISslWebsocketSession>{
			explicit ISslWebsocketSession( beast::ssl_stream<beast::tcp_stream>&& stream ): _ws( std::move(stream) ){}
			α Stream()ι->beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>&{ return _ws; }
		private:
			websocket::stream<beast::ssl_stream<beast::tcp_stream>> _ws;
	};

	template<typename TStream, typename TBody, typename TAllocator>
	[[nodiscard]] α RunWebsocketSession( TStream& stream, beast::flat_buffer& buffer, http::request<TBody, http::basic_fields<TAllocator>> req )->net::awaitable<void, executor_type>{
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

	template<class TBody, class TAllocator>
	α MakeWebsocketSession( beast::tcp_stream stream, http::request<TBody, http::basic_fields<TAllocator>> req )->void{
    std::make_shared<IWebsocketSession>( std::move(stream) )->Run( std::move(req) );
	}
	template<class TBody, class TAllocator>
	α MakeWebsocketSession( beast::ssl_stream<beast::tcp_stream> stream, http::request<TBody, http::basic_fields<TAllocator>> req )->void{
    std::make_shared<ISslWebsocketSession>( std::move(stream) )->Run( std::move(req) );
	}

	template<class TDerived>
	template<class TBody, class TAllocator>
	α TWebSocketSession<TDerived>::DoAccept( http::request<TBody, http::basic_fields<TAllocator>> req )ι->void{
		Derived().Stream().set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );// Set suggested timeout settings for the websocket
		Derived().Stream().set_option( websocket::stream_base::decorator( [](websocket::response_type& res){// Set a decorator to change the Server of the handshake
			res.set( http::field::server, string(BOOST_BEAST_VERSION_STRING) + " advanced-server-flex" );
		}));
		Derived().Stream().async_accept( req, beast::bind_front_handler(&TWebSocketSession::OnAccept, Derived().shared_from_this()) );
	}
}

