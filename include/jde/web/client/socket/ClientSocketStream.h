#pragma once
#include <deque>
#include "../usings.h"

namespace Jde::Web::Client{
	struct IWebsocketSession;
	struct SocketStream;
	struct IClientSocketSession;
	//TODO consider weak_ptr to session
	struct ClientSocketStream final: std::enable_shared_from_this<ClientSocketStream>{
	  using BaseStream = beast::tcp_stream;
		using SslStream = websocket::stream<beast::ssl_stream<BaseStream>>;
		using Stream = std::variant<websocket::stream<BaseStream>,SslStream>;
		ClientSocketStream( net::io_context& ioc, optional<ssl::context>& ctx )ι;
		~ClientSocketStream()=default;

		α OnResolve( tcp::resolver::results_type results, sp<IClientSocketSession> session )ι->void;
		α OnConnect( tcp::resolver::results_type::endpoint_type ep, string& host, sp<IClientSocketSession> session )ι->void;
		α AfterHandshake( const string& host, sp<IClientSocketSession> session )ι->void;
		α AsyncRead( sp<IClientSocketSession> session )ι->void;
		α AsyncWrite( string buffer, sp<IClientSocketSession> session )ι->void;
		α Close( sp<IClientSocketSession> session, bool terminate, SRCE )ι->void;
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;
		α ReadBuffer()ι{ return std::span<uint8_t>{(uint8_t*)_buffer.data().data(), _buffer.size()}; }
		α IsSsl()ι->bool{ return _ws.index()==1; }
		//Close early-returns on an already-closing stream without completing anyone's await, so callers have to ask first.
		α IsClosing()Ι->bool{ return _closing.test(); }
	private:
		α DoWrite()ι->void;//strand. Starts the queue head; OnWrite starts the next.
		α DoClose()ι->void;//strand. Runs once, and only with no write outstanding - _closeSession is the token.
		α Strand()ι->net::any_io_executor{ return std::visit( [](auto&& ws){ return net::any_io_executor{ws.get_executor()}; }, _ws ); }

		beast::flat_buffer _buffer;
		std::deque<string> _writeQueue;//strand-confined.  deque, not vector: async_write holds a reference into front().
		bool _writing{};//strand-confined. An async_write is outstanding - async_close must not overlap it.
		sp<IClientSocketSession> _closeSession;//strand-confined. Held from Close until OnClose; also DoClose's once-only token.
		bool _terminate{};//strand-confined. Close's close-code choice, parked until the close actually starts.
		sp<net::steady_timer> _closeDeadline;//strand-confined. #13 - the deadline the server side has always had and this one did not.
		bool _transportClosed{};//strand-confined. Close deadline fired - lowest layer closed, no close handshake possible.
		std::atomic_flag _closing;//set by Close: later writers drop instead of overlapping the close (a write-type op).  Atomic, not strand-confined - IsClosing() is read off-strand by CloseClientSocketSessionAwait::await_ready.
		Stream _ws;
	};
}