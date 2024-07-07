#pragma once
#include "usings.h"
#include "../../../Framework/source/threading/Mutex.h"

namespace Jde::Http{
	struct IWebsocketSession;
	struct SocketStream;
	struct IClientSocketSession;
/*struct RestStream final: std::enable_shared_from_this<RestStream>{
		RestStream( up<StreamType>&& plain )ι:Plain{ move(plain) }{}
		RestStream( up<beast::ssl_stream<StreamType>>&& ssl )ι:Ssl{ move(ssl) }{}
		RestStream( RestStream&& rhs ):Plain{ move(rhs.Plain) }, Ssl{ move(rhs.Ssl) }{}
		α operator=( RestStream&& rhs )ι->RestStream&;
		α AsyncWrite( http::message_generator&& m )ι->void;
	private:
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;
	public:
		up<StreamType> Plain;//TODO move to variant.
		up<beast::ssl_stream<StreamType>> Ssl;
		friend struct SocketStream;
	};*/
	//template<class TStream=beast::tcp_stream>// client=beast::tcp_stream vs server=beast::tcp_stream::rebind_executor<executor_with_default>::other
	//TODO consider weak_ptr to session
	struct HttpSocketStream final: std::enable_shared_from_this<HttpSocketStream>{
	  using BaseStream = beast::tcp_stream;
		using SslStream = websocket::stream<beast::ssl_stream<BaseStream>>;
		using Stream = std::variant<websocket::stream<BaseStream>,SslStream>;
		HttpSocketStream( net::io_context& ioc, optional<ssl::context>& ctx )ι;

		α OnResolve( tcp::resolver::results_type results, sp<IClientSocketSession> session )ι->void;
		α OnConnect( tcp::resolver::results_type::endpoint_type ep, string& host, sp<IClientSocketSession> session )ι->void;
		α AfterHandshake( const string& host, sp<IClientSocketSession> session )ι->void;
		α AsyncRead( sp<IClientSocketSession> session )ι->void;
		α AsyncWrite( string&& buffer, sp<IClientSocketSession> session )ι->Task;
		α Close( sp<IClientSocketSession> session )ι->void;
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;
		// α OnRun( sp<IWebsocketSession> session )ι->void;
		// α DoRead( sp<IWebsocketSession> session )ι->void;
		α ReadBuffer()ι{ return std::basic_string_view<uint8_t>{(uint8_t*)_buffer.data().data(), _buffer.size()}; }
		α IsSsl()ι->bool{ return _ws.index()==1; }
	private:
		beast::flat_buffer _buffer;
		CoLock _writeLock;
		net::io_context& _ioc;
		up<CoGuard> _writeGuard;
		string _writeBuffer;
		Stream _ws;
	};
}