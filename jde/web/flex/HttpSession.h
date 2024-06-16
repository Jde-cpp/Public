#pragma once

namespace Jde::Web::Flex{
/*
	template<class TDerived>
	struct THttpSession{
	public:
		THttpSession( beast::flat_buffer buffer/*, sp<const std::string> const& doc_root* / ): 
	//		doc_root_(doc_root), 
			_buffer( move(buffer) )
		{}

		α DoRead()->void{
			_parser.emplace();// Construct a new parser for each message
			_parser->body_limit( 4096*2 ); // Apply a reasonable limit to the allowed size of the body in bytes to prevent abuse.
			beast::get_lowest_layer( Derived().Stream()).expires_after(std::chrono::seconds(30) ); // Set the timeout.
			http::async_read( Derived().Stream(), _buffer, *_parser, beast::bind_front_handler( &THttpSession::OnRead, Derived().shared_from_this()) );
		}

		α OnRead(beast::error_code ec, uint bytes_transferred)->void{
			boost::ignore_unused(bytes_transferred);
			if( ec == http::error::end_of_stream )// This means they closed the connection
				return Derived().DoEof();
			if(ec)
				return fail(ec, "read");
	
			if( websocket::is_upgrade(_parser->get()) ){// See if it is a WebSocket Upgrade
				beast::get_lowest_layer(Derived().Stream()).expires_never();// Disable the timeout. The websocket::stream uses its own timeout settings.
				return make_websocket_session( Derived().ReleaseStream(), _parser->release() ); //// Create a websocket session, transferring ownership of both the socket and the HTTP request.
			}
			QueueWrite( HandleRequest(*doc_root_, _parser->release()) );
			if( _responseQueue.size() < QueueLimit )
				DoRead();
		}

		α QueueWrite(http::message_generator response)->void{
			_responseQueue.push( move(response) );// Allocate and store the work
			if( _responseQueue.size() == 1 )// If there was no previous work, start the write loop
				DoWrite();
		}

		α DoWrite()->void{ // Called to start/continue the write-loop. Should not be called when  write_loop is already active. 
			if( !_responseQueue.empty() ){
				bool keepAlive = _responseQueue.front().keep_alive();
				beast::async_write( Derived().Stream(), move(_responseQueue.front()), beast::bind_front_handler(&THttpSession::OnWrite, Derived().shared_from_this(), keepAlive) );
			}
		}

		α OnWrite( bool keep_alive, beast::error_code ec, uint bytes_transferred )->void{
			boost::ignore_unused( bytes_transferred );
			if(ec)
				return fail(ec, "write");
			if( !keep_alive )// This means we should close the connection, usually because the response indicated the "Connection: close" semantic.
				return Derived().DoEof();
			if( _responseQueue.size() == QueueLimit )// Resume the read if it has been paused
				DoRead();
			_responseQueue.pop();
			DoWrite();
		}
	protected:
		beast::flat_buffer _buffer;
	private:
		static constexpr uint QueueLimit = 8;
		α Derived()->TDerived&{ return static_cast<TDerived&>(*this); }
		std::queue<http::message_generator> _responseQueue;
		boost::optional<http::request_parser<http::string_body>> _parser;// The parser is stored in an optional container to construct it from scratch it at the beginning of each new message.
	};

	//non-ssl
	struct IHttpSession : THttpSession<IHttpSession>, std::enable_shared_from_this<IHttpSession>{
    IHttpSession( beast::tcp_stream&& stream, beast::flat_buffer&& buffer, sp<std::string const> const& doc_root ): 
			THttpSession<IHttpSession>( move(buffer), doc_root ), 
			_stream( move(stream) )
    {}

    α Run()->void{ this->DoRead(); }    // Start the session
    α Stream()->beast::tcp_stream&{ return _stream; }
    α ReleaseStream()->beast::tcp_stream{ return move( _stream ); }
    α DoEof()->void{// Send a TCP shutdown
      beast::error_code ec;
      _stream.socket().shutdown( tcp::socket::shutdown_send, ec );// At this point the connection is closed gracefully
    }
		private:
	    beast::tcp_stream _stream;
	};

	// SSL HTTP
	struct ISslHttpSession : THttpSession<ISslHttpSession>, std::enable_shared_from_this<ISslHttpSession>{
		ISslHttpSession( beast::tcp_stream&& stream, ssl::context& ctx, beast::flat_buffer&& buffer, sp<std::string const> const& doc_root ): 
			THttpSession<ISslHttpSession>( move(buffer), doc_root ), 
			stream_(move(stream), ctx)
		{}
		α Run()->void{
			beast::get_lowest_layer(_stream).expires_after( std::chrono::seconds(30) );	// Set the timeout.
			_stream.async_handshake( ssl::stream_base::server, _buffer.data(), beast::bind_front_handler(&ISslHttpSession::OnHandshake, shared_from_this()) );// Perform the SSL handshake.  Note, this is the buffered version of the handshake.
		}
		α Stream()->beast::ssl_stream<beast::tcp_stream>&{ return _stream; }
		α ReleaseStream()->beast::ssl_stream<beast::tcp_stream>{ return move(_stream); }
		α DoEof()->void{
			beast::get_lowest_layer( _stream ).expires_after( std::chrono::seconds(30) );// Set the timeout.
			_stream.async_shutdown( beast::bind_front_handler(&ISslHttpSession::OnShutdown, shared_from_this()) );// Perform the SSL shutdown
		}
	private:
		α OnHandshake( beast::error_code ec, uint bytes_used )->void{
			if(ec)
				return fail( ec, "handshake" );
			_buffer.consume( bytes_used );// Consume the portion of the buffer used by the handshake
			do_read();
		}
		α OnShutdown( beast::error_code ec )->void{
			if(ec)
				return fail(ec, "shutdown"); // At this point the connection is closed gracefully
		}
		beast::ssl_stream<beast::tcp_stream> _stream;
	};
	*/
}