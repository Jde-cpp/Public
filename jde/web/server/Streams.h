#pragma once
#include <boost/beast/ssl/ssl_stream.hpp>
#include "usings.h"
#include <jde/coroutine/Task.h>
#include "../../../../Framework/source/threading/Mutex.h"

namespace Jde::Web::Server{
	using namespace Jde::Coroutine;
	struct IWebsocketSession;
	struct SocketStream;
	struct RestStream final: std::enable_shared_from_this<RestStream>{
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
	};

	struct SocketStream final: std::enable_shared_from_this<SocketStream>{
		using Stream = std::variant<websocket::stream<StreamType>,websocket::stream<beast::ssl_stream<StreamType>>>;
		SocketStream( sp<RestStream>&& stream, beast::flat_buffer&& buffer )ι;
		α Write( string&& buffer )ι->Coroutine::Task;
		α GetExecutor()ι->executor_type;
		α DoAccept( TRequestType request, sp<IWebsocketSession> session )ι->void;
		α DoRead( sp<IWebsocketSession> session )ι->void;
		α Close( sp<IWebsocketSession> session )ι->void;
	private:
		beast::flat_buffer _buffer;
		CoLock _writeLock;
		Stream _ws;
	};
}