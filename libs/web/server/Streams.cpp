#include "Streams.h"
#include <boost/asio/steady_timer.hpp>
#include <jde/web/server/IWebsocketSession.h>
#include <jde/web/server/HttpRequest.h>

#define let const auto
namespace Jde::Web::Server{
	constexpr auto _closeTimeout{ 5s };//how long Close waits for a pending write & the close handshake before tearing down the transport.
	α IRestStream::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), ELogTags::HttpClientWrite, ec.value()==(int)boost::beast::error::timeout ? ELogLevel::Debug : ELogLevel::Error };
	}

#define $ template<class TStream> auto RestStream<TStream>
	$::AsyncWrite( http::message_generator&& m )ι->void{
		beast::async_write( _stream, move(m), beast::bind_front_handler(&RestStream::OnWrite, shared_from_this()) );//&RestStream:: not &IRestStream:: - OnWrite is protected.
	}

	$::CreateSocketStream( beast::flat_buffer&& buffer )ι->sp<ISocketStream>{
		return ms<SocketStream<websocket::stream<TStream>>>( move(_stream), move(buffer) );
	}
#undef $

	template<class TStream>
	SocketStream<TStream>::SocketStream( typename TStream::next_layer_type&& next, beast::flat_buffer&& buffer )ι:
		ISocketStream{ next.get_executor(), move(buffer) },
		_ws{ move(next) }
	{
		_ws.binary( true );
	}

#define $ template<class TStream> auto SocketStream<TStream>
	$::DoAccept( TRequestType req, sp<IWebsocketSession> session )ι->void{
		net::dispatch( _strand, [this, self=shared_from_this(), req=move(req), session=move(session)]()mutable{
			if( _closing )
				return;
			_ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );
			_ws.set_option( websocket::stream_base::decorator( []( websocket::response_type& res ){
				res.set( http::field::server, ServerVersion(IsSsl) );
			}) );
			_ws.async_accept( req, net::bind_executor(_strand, [this, self, session]( beast::error_code ec ){
				_open = !ec;
				session->OnAccept( ec );
			}) );
		});
	}

	$::DoRead( sp<IWebsocketSession> session )ι->void{
		net::dispatch( _strand, [this, self=shared_from_this(), session=move(session)]()mutable{
			if( _closing )
				return;
			_ws.async_read( _buffer, net::bind_executor(_strand, [this, self, session]( beast::error_code ec, uint /*c*/ )mutable{
				if( ec ){
					//ELogLevel level = ec==boost::beast::error::timeout || ec==websocket::error::closed || ec==net::error::connection_aborted || ec==net::error::not_connected || ec==net::error::connection_reset ? ELogLevel::Trace : ELogLevel::Error;
					constexpr ELogLevel level = ELogLevel::Debug;
					if( ec == websocket::error::closed ){
						CodeException{ static_cast<std::error_code>(ec), ELogTags::SocketClientRead, Ƒ("[{:x}]Server::DoRead", session->Id()), level };
						session->OnClose();
					}else
						session->OnDisconnect( CodeException{static_cast<std::error_code>(ec), ELogTags::SocketClientRead, Ƒ("[{:x}]Server::DoRead", session->Id()), level} );
					return;
				}
				session->OnRead( (char*)_buffer.data().data(), _buffer.size() );
				_buffer.clear();
				session->DoRead();
			}) );
		});
	}

	$::Write( string&& output, sp<IWebsocketSession> session )ι->LockAwait::Task{
		auto self = shared_from_this();//the coroutine frame keeps the stream alive across the hop - OnClose can drop the session's ref while we're queued.
		auto outputPtr = mu<string>( move(output) );
		co_await StrandAwait{ _strand };
		auto lock = co_await _writeLock.Lock();
		if( _closing )
			co_return;
		let buffer = net::buffer( (const void*)outputPtr->data(), outputPtr->size() );
		_ws.async_write( buffer, net::bind_executor(_strand, [this, pKeepAlive=move(self), session=move(session), buffer, l=move(lock), out=move(outputPtr) ]( beast::error_code ec, uint bytes_transferred )mutable{
			l.unlock();
			if( ec || out->size()!=bytes_transferred ){
				DBGT( ELogTags::SocketClientWrite | ELogTags::ExternalLogger, "({})Error writing to Session:  '{}'", ec.value(), boost::diagnostic_information(ec) );
				Close( move(session) );
				CodeException{ ec, ELogTags::SocketClientRead };
			}
			(void)buffer;
		}) );
	}

	$::Close( sp<IWebsocketSession> session )ι->LockAwait::Task{
		auto self = shared_from_this();//the coroutine frame keeps the stream alive across the hop.
		co_await StrandAwait{ _strand };
		if( _closing )
			co_return;
		_closing = true;
		if( !_open ){//handshake hasn't completed - async_close is invalid; abort the pending accept at the transport, OnAccept eats the error.
			beast::get_lowest_layer( _ws ).close();
			session->OnClose();
			co_return;
		}
		//Write holds _writeLock across async_write and the close frame is itself a write: a peer that stopped reading (TCP
		//backpressure) would stall either one indefinitely and wedge shutdown at the executor drain. On deadline, close the
		//transport - the aborted op completes on the strand, releasing the lock/finishing the close.
		auto deadline = ms<net::steady_timer>( _strand, _closeTimeout );
		deadline->async_wait( net::bind_executor(_strand, [this, self, deadline]( beast::error_code ec ){
			if( ec )
				return;//cancelled - close completed in time.
			_transportClosed = true;
			beast::get_lowest_layer( _ws ).close();
		}) );
		auto lock = co_await _writeLock.Lock();//async_close is a write op - can't overlap pending writes.
		if( _transportClosed ){//a stalled write ate the deadline - transport is gone, no close handshake possible.
			session->OnClose();
			co_return;
		}
		_ws.async_close( websocket::close_code::normal, net::bind_executor(_strand, [pKeepAlive=move(self), deadline=move(deadline), session=move(session), l=move(lock)]( beast::error_code ec )mutable{
			deadline->cancel();
			l.unlock();
			if( ec )
				CodeException{ static_cast<std::error_code>(ec), ELogTags::SocketClientRead };
			session->OnClose();
		}) );
	}
	template struct SocketStream<websocket::stream<StreamType>>;
	template struct SocketStream<websocket::stream<beast::ssl_stream<StreamType>>>;
	template struct RestStream<StreamType>;
	template struct RestStream<beast::ssl_stream<StreamType>>;
}
