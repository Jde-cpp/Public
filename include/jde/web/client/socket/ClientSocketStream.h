#pragma once
#include "../usings.h"
#include "../../../../../../Framework/source/coroutine/CoLock.h"

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

		α OnResolve( tcp::resolver::results_type results, sp<IClientSocketSession> session )ι->void;
		α OnConnect( tcp::resolver::results_type::endpoint_type ep, string& host, sp<IClientSocketSession> session )ι->void;
		α AfterHandshake( const string& host, sp<IClientSocketSession> session )ι->void;
		α AsyncRead( sp<IClientSocketSession> session )ι->void;
		α AsyncWrite( string&& buffer, sp<IClientSocketSession> session )ι->LockAwait::Task;
		α Close( sp<IClientSocketSession> session )ι->void;
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;
		α ReadBuffer()ι{ return std::basic_string_view<uint8_t>{(uint8_t*)_buffer.data().data(), _buffer.size()}; }
		α IsSsl()ι->bool{ return _ws.index()==1; }
	private:
		beast::flat_buffer _buffer;
		CoLock _writeLock;
		net::io_context& _ioc;
		optional<CoGuard> _writeGuard;
		string _writeBuffer;
		Stream _ws;
	};
}