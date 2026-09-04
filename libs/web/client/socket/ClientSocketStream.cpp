#include <jde/web/client/socket/ClientSocketStream.h>
#include <jde/web/client/ClientSsl.h>
#include <jde/web/client/socket/IClientSocketSession.h>
#include "webClientUtils.h"

#define let const auto
namespace Jde::Web::Client{
	constexpr ELogTags _connectTag{ ELogTags::Socket | ELogTags::Client };
	constexpr auto _closeTimeout{ 5s };//#13: how long Close waits for a pending write & the close handshake before tearing down the transport.  Same value as the server's Streams.cpp.
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
			ws.async_handshake( host, session->_target, beast::bind_front_handler(&IClientSocketSession::OnHandshake, session) );// Perform the websocket handshake - the session's target, "/" unless it asked for a path.
		}, _ws );
	}

	α ClientSocketStream::AsyncRead( sp<IClientSocketSession> session )ι->void{
		_buffer.consume( _buffer.size() );
		std::visit( [this,&session](auto&& ws)->void {
			ws.async_read( _buffer, beast::bind_front_handler(&IClientSocketSession::OnRead, session) );
		}, _ws );
	}

	α ClientSocketStream::AsyncWrite( string buffer, sp<IClientSocketSession> /*session*/ )ι->void{
		net::dispatch( Strand(), [self=shared_from_this(), buffer=move(buffer)]()mutable{//self keeps the stream (and the variant the ops refer into) alive across the hop.
			if( self->_closing.test() )
				return;//close initiated: beast forbids overlapping write-type ops, so drop the frame.
			self->_writeQueue.push_back( move(buffer) );
			if( !self->_writing )
				self->DoWrite();//else OnWrite picks this up.
		});
	}

	//strand.  One outstanding async_write at a time - the queue is what serializes them, and OnWrite drives the next.
	α ClientSocketStream::DoWrite()ι->void{
		_writing = true;
		std::visit( [this](auto&& ws)->void {
			ws.async_write( net::buffer(_writeQueue.front()), beast::bind_front_handler(&ClientSocketStream::OnWrite, shared_from_this()) );
		}, _ws );
	}

	α ClientSocketStream::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{//strand - beast completes through the stream's executor.
		boost::ignore_unused( bytes_transferred );
		_writeQueue.pop_front();
		_writing = false;
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), ELogTags::SocketClientWrite };//TODO look at returning an error to caller.
		if( _closeSession )//Close arrived while this write was outstanding and left the finish to us.
			DoClose();
		else if( !_writeQueue.empty() && !_closing.test() )
			DoWrite();
	}

	α ClientSocketStream::Close( sp<IClientSocketSession> session, bool terminate, SL )ι->void{
		if( _closing.test_and_set() )
			return;//already closing - a second async_close would overlap the first (write-type op).
		DBGT( _connectTag, "[{}]Client::Close: {}", hex(session->Id()), session->Host() );
		net::dispatch( Strand(), [self=shared_from_this(), session=move(session), terminate]()mutable{
			self->_closeSession = move( session );
			self->_terminate = terminate;
			//#13: the server has had this deadline since its own Close rework; this side had none.  AfterHandshake sets
			//expires_never() on the tcp layer, so an async_write to a peer whose receive window is full never completes - OnWrite
			//never runs, DoClose never starts, OnClose never fires, and IClientSocketSession::Shutdown's BlockVoidAwait hangs the
			//process.  On deadline tear the transport down: the aborted op completes on the strand and DoClose finishes without a
			//handshake.  self keeps the stream alive until the timer resolves either way.
			self->_closeDeadline = ms<net::steady_timer>( self->Strand(), _closeTimeout );
			self->_closeDeadline->async_wait( [self]( beast::error_code ec ) {
				if( ec )
					return;//cancelled - the close completed in time.
				self->_transportClosed = true;
				std::visit( [](auto&& ws)->void{ beast::get_lowest_layer(ws).close(); }, self->_ws );
			});
			if( !self->_writing )
				self->DoClose();//else OnWrite runs it - async_close must not overlap a pending write.
		});
	}

	//strand.  _closeSession is the once-only token, and the caller's OnClose is what completes CloseClientSocketSessionAwait.
	α ClientSocketStream::DoClose()ι->void{
		auto session = move( _closeSession );
		if( !session )
			return;
		_writeQueue.clear();//nothing queued can go out now.
		if( _transportClosed ){//#13: a stalled write ate the deadline - the transport is gone, so no close handshake is possible.
			session->OnClose( beast::error::timeout );
			return;
		}
		std::visit( [this,&session](auto&& ws)->void {
			ws.async_close( _terminate ? websocket::close_code::going_away : websocket::close_code::normal, [self=shared_from_this(), session]( beast::error_code ec ){
				if( self->_closeDeadline )
					self->_closeDeadline->cancel();//#13: completed in time - stand the deadline down before it tears the transport out from under a finished close.
				session->OnClose( ec );
			} );
		}, _ws );
	}
}