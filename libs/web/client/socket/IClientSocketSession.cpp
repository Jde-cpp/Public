#include <jde/web/client/socket/IClientSocketSession.h>
#include "boost/asio/error.hpp"
#include "boost/beast/core/error.hpp"
#include "boost/beast/websocket/error.hpp"
#include "jde/fwk.h"
#include "jde/fwk/co/Await.h"
#include "jde/fwk/log/logTags.h"
#include "jde/fwk/usings.h"
#include <jde/app/client/clientSubscriptions.h>

namespace Jde::Web{
	constexpr ELogTags _connectTag{ ELogTags::Socket | ELogTags::Client };
	constexpr ELogTags _connectPedanticTag{ ELogTags::Socket | ELogTags::Client | ELogTags::Pedantic };
	constexpr ELogTags _writeTag{ ELogTags::SocketClientWrite };
	constexpr ELogTags _readTag{ ELogTags::SocketClientRead };

	static optional<uint16> _maxLogLength;
	α Client::MaxLogLength()ι->uint16{
		if( !_maxLogLength )
			_maxLogLength = Settings::FindNumber<uint16>( "http/maxLogLength" ).value_or( 255 );
		return *_maxLogLength;
	}
}
#define CHECK_EC( tag ) if( ec ){ \
	CodeException e{ static_cast<std::error_code>(ec), tag, GetLogLevel(ec) }; \
	if( auto h = _connectHandle; h ){ \
		_connectHandle = nullptr; \
		h.promise().SetExp( move(e) ); \
		h.resume(); \
	}\
	return; \
}
namespace Jde::Web::Client{
	α GetLogLevel( beast::error_code ec )->ELogLevel{
		if( ec==net::error::operation_aborted || ec==boost::beast::websocket::error::closed )
			return ELogLevel::Debug;
		if( ec==boost::asio::error::eof || ec==boost::asio::error::connection_reset ) // server down.
			return ELogLevel::Information;
		return ELogLevel::Error;
	}

	α IClientSocketSession::Shutdown( bool terminate, SL sl )ι->void{
		if( _ioContext ){
			TRACET( _connectTag, "[{}]Client::Shutdown: {}", hex(Id()), Host() );
			BlockVoidAwait( Close(terminate, sl) );
		}
	}

	α IClientSocketSession::AddTask( RequestId requestId, std::any hCoroutine )ι->void{
		_tasks.emplace( requestId, hCoroutine );
	}

	α IClientSocketSession::PopTask( RequestId requestId )ι->std::any{
		std::any h;
		_tasks.erase_if( requestId, [&h](auto&& kv){h=kv.second; return true;} );//Subscriptions aren't in tasks.
		return h;
	}
	//60s: long enough that a slow query is not mistaken for a dead peer, short enough that a stranded caller does not wait out the
	//process.  Whether it is right depends on the workload, hence the setting - a legitimate query that outlives it takes the
	//session down and reconnects, which is worse than waiting.
	Duration _socketRequestTimeout{};
	Ω requestTimeout()ι->Duration{
		auto value = _socketRequestTimeout;
		if( value==Duration::zero() )
			_socketRequestTimeout = value = Settings::FindDuration( "/web/client/socketRequestTimeout" ).value_or( std::chrono::seconds(60) );
		return value;
	}

	α IClientSocketSession::CloseOnError( string reason, SL sl )ι->void{
		Exception{ sl, ELogLevel::Error, "[{}]Closing socket: {}", Ƒ("{:x}", Id()), reason };
		if( auto stream = StreamPtr(); stream )
			stream->Close( shared_from_this(), false, sl );
	}

	α IClientSocketSession::AddTimeout( RequestId requestId, SL sl )ι->TimerAwait::Task{
		const auto _ = shared_from_this();//the timer outlives the request; keep us alive so the check below is not on a freed session.
		const auto timeout = requestTimeout();
		auto timer = ms<DurationTimer>( timeout, sl );
		co_await *timer;
		if( !HasTask(requestId) )
			co_return;//answered, or already failed with the session.
		CloseOnError( Ƒ("request {} unanswered after {}", hex(requestId), Chrono::ToString(timeout)), sl );
	}

	α IClientSocketSession::CloseTasks( function<void(std::any&&)> f )ι->void{
		_tasks.erase_if( [ f ](auto&& kv){
			f( move(kv.second) );
			return true;
		});
	}

	CreateClientSocketSessionAwait::CreateClientSocketSessionAwait( sp<IClientSocketSession> session, string host, PortType port, SL sl )ι:
		base{ sl },
		_session{ session },
		_host{ host },
		_port{ port }
	{}

	α CreateClientSocketSessionAwait::Suspend()ι->void{
		_session->Run( _host, _port, _h );
		_session = nullptr;
	}

	atomic<RequestId> _requestId{ 1 };
	α IClientSocketSession::NextRequestId()ι->RequestId{ return _requestId++; }

	IClientSocketSession::IClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		_resolver{ *ioc },
		_stream{ ms<ClientSocketStream>(*ioc, ctx) },
		_ioContext{ ioc }
	{}

	α IClientSocketSession::Run( string host, PortType port, CreateClientSocketSessionAwait::Handle h )ι->void{ // Start the asynchronous operation
		_connectHandle = h;
		_host = host;
		TRACET( _connectPedanticTag, "[{}:{}]resolve socket.", _host, port );
		_resolver.async_resolve( _host, std::to_string(port), beast::bind_front_handler(&IClientSocketSession::OnResolve, shared_from_this()) );
	}

	α IClientSocketSession::OnResolve( beast::error_code ec, tcp::resolver::results_type results )ι->void{
		CHECK_EC( _writeTag )
		TRACET( _connectPedanticTag, "[{}]resolve succeeded.", _host );
		if( auto stream = StreamPtr(); stream )
			stream->OnResolve( results, shared_from_this() );
	}

	α IClientSocketSession::OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type ep )ι->void{
		CHECK_EC( _readTag )
		TRACET( _connectPedanticTag, "[{}]connect succeeded.", _host );
		if( auto stream = StreamPtr(); stream )
			stream->OnConnect( ep, _host, shared_from_this() );
	}

	α IClientSocketSession::OnSslHandshake( beast::error_code ec )ι->void{
		CHECK_EC( _readTag )
		TRACET( _connectPedanticTag, "[{}]SslHandshake succeeded.", _host );
		if( auto stream = StreamPtr(); stream )
			stream->AfterHandshake( _host, shared_from_this() );
	}

	α IClientSocketSession::OnHandshake( beast::error_code ec )ι->void{
		CHECK_EC( _readTag )
		DBGT( _connectTag, "[{}]OnHandshake succeeded. Calling read.", _host );
		if( auto h = _connectHandle; h ){
			_connectHandle = nullptr;
			h.resume();
		}
		if( auto stream = StreamPtr(); stream )
			stream->AsyncRead( shared_from_this() );
	}
	α IClientSocketSession::Write( string&& m )ι->void{
		//the hot cross-thread case: a caller writing while a close is running on the strand used to dereference a nulled _stream.
		if( auto stream = StreamPtr(); stream )
			stream->AsyncWrite( move(m), shared_from_this() );
	}

	α IClientSocketSession::OnRead( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		if( ec ){
			CodeException{ static_cast<std::error_code>(ec), _readTag, Ƒ("[{:x}]ClientSocket::DoRead", Id()), GetLogLevel(ec) };
			if( ec==net::error::operation_aborted )// our own in-flight Close() cancelled this read; its OnClose completion will drain _tasks with the real close reason, so don't preempt it with a misleading "operation_aborted" one here.
				return;
			// websocket::error::closed means the close handshake already completed (Beast auto-replies to a received close frame);
			// calling Close() again would initiate a second async_close that collides with the in-flight one on Beast's write
			// soft_mutex.  Tested first so the stream lookup below needs no nesting - an `else` after an `if` that guards on
			// StreamPtr binds to the inner one.
			if( ec==boost::beast::websocket::error::closed )
				CloseTasks( ec );// remote-initiated close: we never call our own Close()/async_close for this case, so OnClose never fires - drain _tasks here instead.
			else if( auto stream = StreamPtr(); stream )
				stream->Close( shared_from_this(), false, SRCE_CUR );
			return;
		}
		auto stream = StreamPtr();
		if( !stream )
			return;
		OnReadData( stream->ReadBuffer() );
		stream->AsyncRead( shared_from_this() );
	}
	//Nothing to wait on: no stream, or a close already running.  OnClose resumes the *first* waiter and only then nulls _stream, so
	//a caller that closes again on waking finds a live-but-closing stream - and ClientSocketStream::Close early-returns on _closing
	//without completing anyone's await, so that second caller would wait out the process.
	//Answered here rather than resuming from Suspend: calling Resume() inside await_suspend re-enters a coroutine that has not
	//finished suspending.
	α CloseClientSocketSessionAwait::await_ready()ι->bool{
		auto stream = _session->StreamPtr();
		return !stream || stream->IsClosing();
	}
	α CloseClientSocketSessionAwait::Suspend()ι->void{
		_session->_closeHandle = _h;
		if( auto stream = _session->StreamPtr(); stream )
			stream->Close( _session, _terminate );
	}
	α IClientSocketSession::OnClose( beast::error_code ec )ι->void{
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), _readTag, Ƒ("[{}]Client::OnClose: {}", hex(Id()), _host), GetLogLevel(ec) };
		else
			DBGT( _connectTag, "[{}]Client::OnClose: {}", hex(Id()), _host );
		CloseTasks( ec );
		if( _closeHandle )
			_closeHandle.resume();
		_closeHandle = nullptr;
		{
			lg _{ _streamMutex };
			_stream = nullptr;
		}
		_ioContext = nullptr;
	}
}