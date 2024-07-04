#include <jde/web/flex/Streams.h>
#include <jde/web/flex/IWebsocketSession.h>

#define var const auto
namespace Jde::Web::Flex{
	static sp<Jde::LogTag> _logTag = Logging::Tag( "web" );
	static sp<Jde::LogTag> _requestTag = Logging::Tag( "web.request" );
	static sp<Jde::LogTag> _responseTag = Logging::Tag( "web.response" );

	α RestStream::operator=( RestStream&& rhs )ι->RestStream&{
		Plain = move(rhs.Plain);
		Ssl = move(rhs.Ssl);
		return *this;
	}

	α RestStream::AsyncWrite( http::message_generator&& m )ι->void{
		if( Plain )//TODO Move async_write to streams class and pass shared_from_this.  Implement certificates.
			beast::async_write( *Plain, move(m), beast::bind_front_handler(&RestStream::OnWrite, shared_from_this()) );
		else
			beast::async_write( *Ssl, move(m), beast::bind_front_handler(&RestStream::OnWrite, shared_from_this()) );
	}

	α RestStream::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), ec.value()==(int)boost::beast::error::timeout ? ELogLevel::Debug : ELogLevel::Error };
  }

	α Test( beast::ssl_stream<StreamType>&& stream )ι->void{
		websocket::stream<beast::ssl_stream<StreamType>> x{ move(stream) };
	}

	α CreateWS( sp<RestStream>&& stream )ι->optional<SocketStream::Stream>{
		optional<SocketStream::Stream> y;
		//beast::ssl_stream<StreamType>& s2 = *stream.Ssl;
		//websocket::stream<beast::ssl_stream<StreamType>> x{ move(*stream.Ssl) };
		if( stream->Plain )
		 	y.emplace( websocket::stream<StreamType>{ move(*stream->Plain) } );
		else{
      //websocket::stream<beast::ssl_stream<StreamType>> x2{ move(*stream.Ssl) };
		 	y.emplace( websocket::stream<beast::ssl_stream<StreamType>>{ move(*stream->Ssl) } );
		}
		std::visit( [](auto&& ws){ ws.binary(true); }, *y );
		return y;
	}
	SocketStream::SocketStream( sp<RestStream>&& stream, beast::flat_buffer&& buffer )ι:
		_buffer{ move(buffer) },
		_ws{ *CreateWS(move(stream)) }
	{
		//set binary
		//?_ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );
	}
	α SocketStream::GetExecutor()ι->executor_type{
		//executor_type executor;
		return std::visit( [&]( auto&& ws ){	return ws.get_executor(); }, _ws );
		//return executor;
	}
	α SocketStream::DoAccept( TRequestType req, sp<IWebsocketSession> session )ι->void{
		std::visit(
			[&]( auto&& ws ){
				ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );
				ws.set_option( websocket::stream_base::decorator( []( websocket::response_type& res ){
					res.set( http::field::server, string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async" );//TODO fix.
				}) );
				ws.async_accept( req, beast::bind_front_handler(&IWebsocketSession::OnAccept, session) );
			}, _ws );
	}

	α SocketStream::DoRead( sp<IWebsocketSession> session )ι->void{
		std::visit(
			[&]( auto&& ws ){
				ws.async_read( _buffer, [this,session]( beast::error_code ec, uint c )mutable{
					if( ec ){
						ELogLevel level = ec==websocket::error::closed || ec==boost::asio::error::connection_aborted || ec==boost::asio::error::not_connected || ec==boost::asio::error::connection_reset ? ELogLevel::Trace : ELogLevel::Error;
						CodeException{ static_cast<std::error_code>(ec), _requestTag, level };
						return;
					}
					session->OnRead( (char*)_buffer.data().data(), _buffer.size() );
					_buffer.clear();
					session->DoRead();
				});
			}, _ws );
	}

	α SocketStream::Write( string&& output )ι->Task{
	//	output = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		auto outputPtr = mu<string>( move(output) );
		var buffer = net::buffer( (const void*)outputPtr->data(), outputPtr->size() );
		LockAwait await = _writeLock.Lock(); //gcc doesn't like co_await _writeLock.Lock();
		auto lock = ( co_await await ).UP<CoGuard>();
		std::visit(
			[&]( auto&& ws ){
				ws.async_write( buffer, [this, &ws, pKeepAlive=shared_from_this(), buffer, l=move(lock), out=move(outputPtr) ]( beast::error_code ec, uint bytes_transferred )mutable{
					l = nullptr;
					if( ec || out->size()!=bytes_transferred ){
						Logging::LogNoServer( Logging::Message(ELogLevel::Debug, "Error writing to Session:  '{}'"), _responseTag, boost::diagnostic_information(ec) );
						try{
							ws.close( websocket::close_code::none );
						}
						catch( const boost::exception& e2 ){
							Logging::LogNoServer( Logging::Message{ELogLevel::Debug, "Error closing:  '{}')"}, _responseTag, boost::diagnostic_information(e2) );
						}
						CodeException{ move(static_cast<std::error_code>(ec)) };
					}
				});
			}, _ws );
	}

	α SocketStream::Close( sp<IWebsocketSession> session )ι->void{
		std::visit(
			[&]( auto&& ws ){
				ws.async_close( websocket::close_code::normal, [session]( beast::error_code ec ){
					if( ec )
						CodeException{ static_cast<std::error_code>(ec) };
					INFO( "[{:x}]Closed.", session->Id() );
				});
			}, _ws );
	}
}
