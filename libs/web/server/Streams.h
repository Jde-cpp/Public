#pragma once
#include <deque>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <jde/web/server/usings.h>
#include <jde/web/server/exports.h>

namespace Jde::Web::Server{
	struct IWebsocketSession; struct ISocketStream;
	//Non-template handle for the http side - RestStream<TStream> owns the socket once RunSession hands it over, and turns it into the websocket on upgrade.
	struct ΓWS IRestStream: std::enable_shared_from_this<IRestStream>{
		virtual ~IRestStream()=default;
		β AsyncWrite( http::message_generator&& m )ι->void=0;
		β CreateSocketStream( beast::flat_buffer&& buffer )ι->sp<ISocketStream> = 0;
	protected:
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;
	};

	//TStream: StreamType | beast::ssl_stream<StreamType>.  Bodies live in Streams.cpp, explicitly instantiated for both.
	template<class TStream>
	struct RestStream final: IRestStream{
		RestStream( TStream&& stream )ι:_stream{ move(stream) }{}
		α AsyncWrite( http::message_generator&& m )ι->void override;
		α CreateSocketStream( beast::flat_buffer&& buffer )ι->sp<ISocketStream> override;
	private:
		TStream _stream;
	};

	//Non-template handle held by IWebsocketSession - owns everything that doesn't depend on the stream type; SocketStream<TStream> supplies the beast ops.
	struct ΓWS ISocketStream: std::enable_shared_from_this<ISocketStream>{
		virtual ~ISocketStream()=default;
		β Write( string&& buffer, sp<IWebsocketSession> session )ι->void=0;
		β DoAccept( TRequestType request, sp<IWebsocketSession> session )ι->void=0;
		β DoRead( sp<IWebsocketSession> session )ι->void=0;
		β Close( sp<IWebsocketSession> session )ι->void=0;
		α GetExecutor()ι->executor_type{ return _strand.get_inner_executor(); }
		α Strand()ι->net::strand<executor_type>&{ return _strand; }
	protected:
		ISocketStream( executor_type executor, beast::flat_buffer&& buffer )ι:_buffer{ move(buffer) }, _strand{ net::make_strand(executor) }{}
		//#9: a strand-confined write queue, where a CoLock used to be.  The lock was held from before async_write until its
		//completion handler, and a *contended* Lock() resumes its waiter through CoLock::Clear's Post() - the raw io_context -
		//so the next beast op was initiated off-strand, possibly while the strand ran DoRead's completion.  beast streams are not
		//thread-safe.  The queue expresses the same rule (beast allows one outstanding write-type op) without ever leaving the
		//strand: everything below is touched only from a handler on _strand, so there is no lock to contend and no hop back.
		struct WriteItem{
			WriteItem( string&& buffer, sp<IWebsocketSession>&& session )ι:Buffer{ move(buffer) }, Session{ move(session) }{}
			string Buffer;
			sp<IWebsocketSession> Session;//keeps a queued frame's session alive, as the old completion-handler capture did.
		};
		beast::flat_buffer _buffer;
		net::strand<executor_type> _strand;//serializes every op on _ws: the ioc is multithreaded and beast streams aren't thread-safe, so Close (shutdown thread) would otherwise race read/write handlers.
		std::deque<WriteItem> _writeQueue;//strand-confined.  deque, not vector: async_write holds a reference into front() while later frames are appended behind it.
		bool _writing{};//strand-confined. An async_write is outstanding - beast forbids a second write-type op (async_close included) until it completes.  This is what the CoLock expressed.
		sp<IWebsocketSession> _closeSession;//strand-confined. Held from Close until OnClose - a close that has to wait out a write still needs the session afterwards.  Also the once-only token for DoClose.
		sp<net::steady_timer> _closeDeadline;//strand-confined.
		bool _open{};//strand-confined. Handshake completed - async_close is only valid on an open stream.
		bool _closing{};//strand-confined. Close initiated - makes Close idempotent & drops later reads/writes.
		bool _transportClosed{};//strand-confined. Close deadline fired - lowest layer closed, no close handshake possible.
	};

	//TStream: websocket::stream<StreamType> | websocket::stream<beast::ssl_stream<StreamType>>.  Bodies live in Streams.cpp, explicitly instantiated for both.
	template<class TStream>
	struct SocketStream final: ISocketStream{
		static constexpr bool IsSsl = std::is_same_v<TStream, websocket::stream<beast::ssl_stream<StreamType>>>;
		SocketStream( typename TStream::next_layer_type&& next, beast::flat_buffer&& buffer )ι;//next = RestStream<>::_stream
		α Write( string&& buffer, sp<IWebsocketSession> session )ι->void override;
		α DoAccept( TRequestType request, sp<IWebsocketSession> session )ι->void override;
		α DoRead( sp<IWebsocketSession> session )ι->void override;
		α Close( sp<IWebsocketSession> session )ι->void override;
	private:
		α DoWrite()ι->void;//strand. Starts the queue head; its completion starts the next.
		α DoClose()ι->void;//strand. The single OnClose path - runs once, and only with no write outstanding.
		TStream _ws;
	};
}
