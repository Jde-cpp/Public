#include <jde/web/client/socket/ClientSocketStream.h>
#include <jde/web/client/ClientSsl.h>
#include <jde/web/client/socket/IClientSocketSession.h>
#include "webClientUtils.h"

#define let const auto
namespace Jde::Web::Client{
	constexpr ELogTags _connectTag{ ELogTags::Socket | ELogTags::Client };
	static string _userAgent{ Ƒ("({})Jde.Web.Client - {}", Process::ProductVersion, BOOST_BEAST_VERSION) };
	string _sslUserAgent{ Ƒ("({})Jde.Web.Client SSL - {}", Process::ProductVersion, BOOST_BEAST_VERSION) };


	ClientSocketStream::ClientSocketStream( net::io_context& ioc, optional<ssl::context>& ctx )ι:
		_ws{ ctx
			? Stream{ websocket::stream<beast::ssl_stream<BaseStream>>{net::make_strand(ioc), *ctx} }
			: Stream{ websocket::stream<BaseStream>{net::make_strand(ioc)} }}
	{
		std::visit( [](auto&& ws)->void {
			ws.binary(true);
		}, _ws );
	}

	α ClientSocketStream::OnResolve( tcp::resolver::results_type results, sp<IClientSocketSession> session )ι->void{
		auto endpoints = PreferV4( results );// async_connect walks endpoints serially; try IPv4 first to avoid dead-IPv6 connect stalls (see PreferV4).
		std::visit( [&endpoints,session](auto&& ws)->void {
			beast::get_lowest_layer( ws ).expires_after( std::chrono::seconds(30) );
			beast::get_lowest_layer( ws ).async_connect( endpoints, beast::bind_front_handler(&IClientSocketSession::OnConnect, session) );// Make the connection on the IP address we get from a lookup
		},
		_ws);
	}

	α ClientSocketStream::OnConnect( tcp::resolver::results_type::endpoint_type ep, string& host, sp<IClientSocketSession> session )ι->void{
		if( IsSsl() ){
			auto& stream = get<1>( _ws );
			beast::get_lowest_layer( stream ).expires_after( std::chrono::seconds(30) );// Set a timeout on the operation
			if( !SSL_set_tlsext_host_name(stream.next_layer().native_handle(), host.c_str()) ){// Set SNI Hostname (many hosts need this to handshake successfully)
				let ec = beast::error_code( static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() );
				CodeException{ static_cast<std::error_code>(ec), ELogTags::SocketClientRead };
				return;
			}
			Ssl::SetVerifyHost( stream.next_layer(), host );//C1: bind the peer's certificate to the host we dialled, not just to a trusted anchor.
			host += ':' + std::to_string( ep.port() ); // Update the _host string. This will provide the value of the Host HTTP header during the WebSocket handshake. See https://tools.ietf.org/html/rfc7230#section-5.4
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

	α ClientSocketStream::AsyncWrite( string buffer, sp<IClientSocketSession> /*session*/ )ι->LockAwait::Task{
		auto guard = co_await _writeLock.Lock();
		if( _closing.test() )
			co_return;//close initiated: beast forbids overlapping write-type ops, so drop the frame. guard releases here.
		_writeGuard = move( guard );
		_writeBuffer = move( buffer );
		std::visit( [this](auto&& ws)->void {
			//initiate on the stream's strand, not the raw ioc: beast streams are not thread-safe, and an off-strand
			//initiation racing a read completion intermittently loses the write with no completion - OnWrite then never
			//releases _writeLock and every later frame on the session queues forever (soak finding #3).
			//self keeps the stream (and the variant ws refers into) alive until the post runs.
			net::post( ws.get_executor(), [self=shared_from_this(), &ws](){
				ws.async_write( net::buffer(self->_writeBuffer), beast::bind_front_handler(&ClientSocketStream::OnWrite, self) );
			});
		}, _ws );
	}
	α ClientSocketStream::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
		_writeBuffer.clear();
		auto guard = move(_writeGuard);
		guard.reset();
		boost::ignore_unused( bytes_transferred );
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), ELogTags::SocketClientWrite };//TODO look at returning an error to caller.
	}

	α ClientSocketStream::Close( sp<IClientSocketSession> session, bool terminate, SL )ι->LockAwait::Task{
		if( _closing.test_and_set() )
			co_return;//already closing - a second async_close would overlap the first (write-type op).
		DBGT( _connectTag, "[{}]Client::Close: {}", hex(session->Id()), session->Host() );
		auto guard = co_await _writeLock.Lock();//wait out any in-flight write - async_close is a write-type op and must not overlap it.
		std::visit( [this, session, terminate](auto&& ws)->void {
			net::post( ws.get_executor(), [self=shared_from_this(), session, terminate, &ws](){//strand, as in AsyncWrite.
				ws.async_close( terminate ? websocket::close_code::going_away : websocket::close_code::normal, beast::bind_front_handler(&IClientSocketSession::OnClose, session) );
			});
		}, _ws );
		guard.unlock();//_closing keeps later writers from initiating; release so queued writers wake and drop out.
	}
}