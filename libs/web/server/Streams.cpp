#include "Streams.h"
#include <boost/asio/steady_timer.hpp>
#include <jde/web/server/IWebsocketSession.h>
#include <jde/web/server/HttpRequest.h>
#include <jde/web/server/Server.h>

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
			//#16: Beast defaults this to 16 MB, so the socket accepted a query three orders of magnitude past what the http
			//path allows - and the parser recursed through all of it on an 8 MB io thread stack.
			//web-review3 #14: its own limit now, not /graphql's.  A socket message is not an http body - sharing the body cap
			//made any RemoteLog backlog past 10 KB permanently undeliverable.
			_ws.read_message_max( Server::SocketMessageMax() );
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

	$::Write( string&& output, sp<IWebsocketSession> session )ι->void{
		net::dispatch( _strand, [this, self=shared_from_this(), output=move(output), session=move(session)]()mutable{//self keeps the stream alive across the hop - OnClose can drop the session's ref while we're queued.
			if( _closing )
				return;//a close frame is itself a write-type op, so nothing may be initiated once Close has started.
			_writeQueue.emplace_back( move(output), move(session) );
			if( !_writing )
				DoWrite();//else the in-flight write's completion picks this up.
		});
	}

	//strand.  One outstanding async_write at a time - the queue is what serializes them, and its completion drives the next.
	$::DoWrite()ι->void{
		_writing = true;
		_ws.async_write( net::buffer(_writeQueue.front().Buffer), net::bind_executor(_strand, [this, self=shared_from_this()]( beast::error_code ec, uint bytes )mutable{
			let expected = _writeQueue.front().Buffer.size();
			auto session = move( _writeQueue.front().Session );
			_writeQueue.pop_front();
			_writing = false;
			if( ec || expected!=bytes ){
				DBGT( ELogTags::SocketClientWrite | ELogTags::ExternalLogger, "({})Error writing to Session:  '{}'", ec.value(), boost::diagnostic_information(ec) );
				CodeException{ ec, ELogTags::SocketClientRead };
				if( _closing )
					DoClose();//Close already ran and left the finish to us.
				else
					Close( move(session) );//dispatch runs inline - we are on the strand - and finishes, since nothing is outstanding now.
			}
			else if( _closing )//Close arrived while this write was outstanding.
				DoClose();
			else if( !_writeQueue.empty() )
				DoWrite();
		}) );
	}

	$::Close( sp<IWebsocketSession> session )ι->void{
		net::dispatch( _strand, [this, self=shared_from_this(), session=move(session)]()mutable{//self keeps the stream alive across the hop.
			if( _closing )
				return;//idempotent
			_closing = true;
			_closeSession = move( session );
			//A close frame is a write: a peer that stopped reading (TCP backpressure) would stall it - or the write it is queued
			//behind - indefinitely and wedge shutdown at the executor drain.  On deadline, close the transport; the aborted op
			//completes on the strand and DoClose finishes without a handshake.
			if( _open ){
				_closeDeadline = ms<net::steady_timer>( _strand, _closeTimeout );
				_closeDeadline->async_wait( net::bind_executor(_strand, [this, self]( beast::error_code ec ){
					if( ec )
						return;//cancelled - close completed in time.
					_transportClosed = true;
					beast::get_lowest_layer( _ws ).close();
				}) );
			}
			if( !_writing )
				DoClose();//else the write's completion runs it - async_close must not overlap a pending write.
		});
	}

	//strand.  Every ending funnels here - Close(), and a failed write.  _closeSession is the once-only token: OnClose runs exactly once.
	$::DoClose()ι->void{
		auto session = move( _closeSession );
		if( !session )
			return;
		_writeQueue.clear();//nothing queued can go out now - drop the frames, and the session refs they hold, here rather than at destruction.
		if( !_open || _transportClosed ){//handshake never completed, or the deadline already tore the transport down: async_close is invalid either way.
			if( !_transportClosed )
				beast::get_lowest_layer( _ws ).close();//abort the pending accept at the transport; OnAccept eats the error.
			if( _closeDeadline )
				_closeDeadline->cancel();
			session->OnClose();
			return;
		}
		_ws.async_close( websocket::close_code::normal, net::bind_executor(_strand, [this, self=shared_from_this(), session=move(session)]( beast::error_code ec )mutable{
			if( _closeDeadline )
				_closeDeadline->cancel();
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
