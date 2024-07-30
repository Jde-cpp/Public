#include <jde/web/client/socket/ClientSocketStream.h>
#include <jde/web/client/socket/IClientSocketSession.h>

#define var const auto
namespace Jde::Web::Client{
	static string _userAgent{ 𐢜("({})Jde.Web.Client - {}", IApplication::ProductVersion, BOOST_BEAST_VERSION) };
	string _sslUserAgent{ 𐢜("({})Jde.Web.Client SSL - {}", IApplication::ProductVersion, BOOST_BEAST_VERSION) };


	ClientSocketStream::ClientSocketStream( net::io_context& ioc, optional<ssl::context>& ctx )ι:
		_ioc{ ioc },
		_ws{ ctx
			? Stream{ websocket::stream<beast::ssl_stream<BaseStream>>{net::make_strand(ioc), *ctx} }
			: Stream{ websocket::stream<BaseStream>{net::make_strand(ioc)} }}
	{
		std::visit( [](auto&& ws)->void {
			ws.binary(true);
		}, _ws );
	}

	α ClientSocketStream::OnResolve( tcp::resolver::results_type results, sp<IClientSocketSession> session )ι->void{
		std::visit( [this,&results,session](auto&& ws)->void {
			beast::get_lowest_layer( ws ).expires_after( std::chrono::seconds(30) );
			beast::get_lowest_layer( ws ).async_connect( results, beast::bind_front_handler(&IClientSocketSession::OnConnect, session) );// Make the connection on the IP address we get from a lookup
		},
		_ws);
	}

	α ClientSocketStream::OnConnect( tcp::resolver::results_type::endpoint_type ep, string& host, sp<IClientSocketSession> session )ι->void{
		if( IsSsl() ){
			auto& stream = get<1>( _ws );
			beast::get_lowest_layer( stream ).expires_after( std::chrono::seconds(30) );// Set a timeout on the operation
			// Set SNI Hostname (many hosts need this to handshake successfully)
			if( !SSL_set_tlsext_host_name(stream.next_layer().native_handle(), host.c_str()) ){
				var ec = beast::error_code( static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() );
				CodeException{ static_cast<std::error_code>(ec), SocketClientReadTag() };
				return;
			}
			host += ':' + std::to_string( ep.port() ); // Update the _host string. This will provide the value of the Host HTTP header during the WebSocket handshake. See https://tools.ietf.org/html/rfc7230#section-5.4
			// Perform the SSL handshake
			stream.next_layer().async_handshake( ssl::stream_base::client, beast::bind_front_handler( &IClientSocketSession::OnSslHandshake, session) );
		}
		else{
			host += ':' + std::to_string( ep.port() );// Update the host string. This will provide the value of the Host HTTP header during the WebSocket handshake.  See https://tools.ietf.org/html/rfc7230#section-5.4
			AfterHandshake( host, session );
		}
	}
	α ClientSocketStream::AfterHandshake( const string& host, sp<IClientSocketSession> session )ι->void{
		std::visit( [this,&host,session](auto&& ws)->void {
			beast::get_lowest_layer( ws ).expires_never();// Turn off the timeout on the tcp_stream, because the websocket stream has its own timeout system.
			ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::client) );// Set suggested timeout settings for the websocket
			string userAgent = IsSsl() ? _sslUserAgent : _userAgent;
			ws.set_option(websocket::stream_base::decorator( [userAgent](websocket::request_type& req){// Set a decorator to change the User-Agent of the handshake
				req.set( http::field::user_agent, userAgent );
			}));
			ws.async_handshake( host, "/", beast::bind_front_handler(&IClientSocketSession::OnHandshake, session) );// Perform the websocket handshake
		}, _ws );
	}

	α ClientSocketStream::AsyncRead( sp<IClientSocketSession> session )ι->void{
		_buffer.consume( _buffer.size() );
		std::visit( [this,&session](auto&& ws)->void {
			ws.async_read( _buffer, beast::bind_front_handler(&IClientSocketSession::OnRead, session) );
		}, _ws );
	}

	auto _logTag = Logging::Tag( "tests" );
	α ClientSocketStream::AsyncWrite( string&& buffer, sp<IClientSocketSession> /*session*/ )ι->Task{
		LockAwait await = _writeLock.Lock(); //gcc doesn't like co_await _writeLock.Lock();
		_writeGuard = (co_await await).UP<CoGuard>();
		_writeBuffer = move(buffer);
		std::visit( [this](auto&& ws)->void {
			net::post( _ioc, [this, &ws](){
				ws.async_write( net::buffer(_writeBuffer), beast::bind_front_handler(&ClientSocketStream::OnWrite, shared_from_this()) );
			});
		}, _ws );
	}
	α ClientSocketStream::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
		_writeBuffer.clear();
		_writeGuard = nullptr;
		boost::ignore_unused( bytes_transferred );
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), SocketClientWriteTag() };
	}

	α ClientSocketStream::Close( sp<IClientSocketSession> session )ι->void{
		std::visit( [session](auto&& ws)->void {
			ws.async_close( websocket::close_code::normal, beast::bind_front_handler(&IClientSocketSession::OnClose, session) );
		}, _ws );
	}
}