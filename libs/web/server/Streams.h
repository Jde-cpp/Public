#pragma once
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <jde/web/server/usings.h>
#include <jde/web/server/exports.h>
#include <jde/fwk/co/CoLock.h>

namespace Jde::Web::Server{
//	using namespace Jde::Coroutine;
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

	//Resumes the coroutine on the strand - promise-agnostic (unlike VoidAwait/IAwait) so it can be awaited inside LockAwait::Task.
	struct StrandAwait{
		net::strand<executor_type>& Strand;
		α await_ready()Ι->bool{ return Strand.running_in_this_thread(); }
		α await_suspend( coroutine_handle<> h )Ι->void{ net::post( Strand, [h]{ h.resume(); } ); }
		α await_resume()Ι->void{}
	};

	struct ΓWS SocketStream final: std::enable_shared_from_this<SocketStream>{
		using Stream = std::variant<websocket::stream<StreamType>,websocket::stream<beast::ssl_stream<StreamType>>>;
		SocketStream( sp<RestStream>&& stream, beast::flat_buffer&& buffer )ι;
		α Write( string&& buffer, sp<IWebsocketSession> session )ι->LockAwait::Task;
		α GetExecutor()ι->executor_type;
		α Strand()ι->net::strand<executor_type>&{ return _strand; }
		α DoAccept( TRequestType request, sp<IWebsocketSession> session )ι->void;
		α DoRead( sp<IWebsocketSession> session )ι->void;
		α Close( sp<IWebsocketSession> session )ι->LockAwait::Task;
	private:
		beast::flat_buffer _buffer;
		CoLock _writeLock;
		Stream _ws;
		net::strand<executor_type> _strand;//serializes every op on _ws: the ioc is multithreaded and beast streams aren't thread-safe, so Close (shutdown thread) would otherwise race read/write handlers.
		bool _open{};//strand-confined. Handshake completed - async_close is only valid on an open stream.
		bool _closing{};//strand-confined. Close initiated - makes Close idempotent & drops later reads/writes.
		bool _transportClosed{};//strand-confined. Close deadline fired - lowest layer closed, no close handshake possible.
	};
}
