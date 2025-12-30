#include <jde/web/client/socket/IClientSocketSession.h>
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
#define CHECK_EC(tag) if( ec ){ \
	CodeException e{ static_cast<std::error_code>(ec), tag, GetLogLevel(ec) }; \
	if( _connectHandle ){ \
		_connectHandle.promise().SetExp( move(e) ); \
		_connectHandle.resume(); \
		return; \
	}\
}
namespace Jde::Web::Client{
	α GetLogLevel( beast::error_code ec )->ELogLevel{
		return ec == net::error::operation_aborted
			? ELogLevel::Trace
			: ELogLevel::Error;
	}

	α IClientSocketSession::AddTask( RequestId requestId, std::any hCoroutine )ι->void{
		_tasks.emplace( requestId, hCoroutine );
	}

	α IClientSocketSession::PopTask( RequestId requestId )ι->std::any{
		std::any h;
		_tasks.erase_if( requestId, [&h](auto&& kv){ h=kv.second; return true;} );//Subscriptions aren't in tasks.
		return h;
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
	}

	atomic<RequestId> _requestId{ 1 };
	α IClientSocketSession::NextRequestId()ι->RequestId{ return _requestId++; }

	IClientSocketSession::IClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		_resolver{ *ioc },
		_stream{ ms<ClientSocketStream>(*ioc, ctx) },
		_ioContext{ ioc }
	{}

	α IClientSocketSession::Run( string host, PortType port, CreateClientSocketSessionAwait::Handle h )ι->void{// Start the asynchronous operation
		_connectHandle = h;
		_host = host;
		net::post( *_ioContext, [=, self=shared_from_this()]{
			TRACET( _connectPedanticTag, "[{}:{}]resolve socket.", self->_host, port );
			beast::error_code ec;
			auto results = self->_resolver.resolve( self->_host, std::to_string(port), ec );//async_resolve starts another thread.
			//_resolver.async_resolve( _host, std::to_string(port_), beast::bind_front_handler(&IClientSocketSession::OnResolve, shared_from_this()) );// Look up the domain name
			self->OnResolve( ec, results );
		});
	}

	α IClientSocketSession::OnResolve( beast::error_code ec, tcp::resolver::results_type results )ι->void{
		CHECK_EC( _writeTag )
		TRACET( _connectPedanticTag, "[{}]resolve succeeded.", _host );
		_stream->OnResolve( results, shared_from_this() );
	}

	α IClientSocketSession::OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type ep )ι->void{
		CHECK_EC( _readTag )
		TRACET( _connectPedanticTag, "[{}]connect succeeded.", _host );
		_stream->OnConnect( ep, _host, shared_from_this() );
	}

	α IClientSocketSession::OnSslHandshake( beast::error_code ec )ι->void{
		CHECK_EC( _readTag )
		TRACET( _connectPedanticTag, "[{}]SslHandshake succeeded.", _host );
		_stream->AfterHandshake( _host, shared_from_this() );
	}

	α IClientSocketSession::OnHandshake( beast::error_code ec )ι->void{
		CHECK_EC( _readTag )
		DBGT( _connectTag, "[{}]OnHandshake succeeded. Calling read.", _host );
		if( _connectHandle )
			_connectHandle.resume();
		_stream->AsyncRead( shared_from_this() );
	}
	α IClientSocketSession::Write( string&& m )ι->void{
		_stream->AsyncWrite( move(m), shared_from_this() );
	}

	α IClientSocketSession::OnRead( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		if( ec ){
			CodeException{ static_cast<std::error_code>(ec), _readTag, Ƒ("[{:x}]ClientSocket::DoRead", Id()), GetLogLevel(ec) };
			if( ec!=net::error::operation_aborted )
				_stream->Close( shared_from_this() );
			return;
		}
		OnReadData( _stream->ReadBuffer() );
		_stream->AsyncRead( shared_from_this() );
	}
	α CloseClientSocketSessionAwait::Suspend()ι->void{
		_session->_closeHandle = _h;
		_session->_stream->Close( _session );
	}
	α IClientSocketSession::OnClose( beast::error_code ec )ι->void{
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), _readTag, Ƒ("[{:x}]Client::OnClose", Id()), GetLogLevel(ec) };
		else
			TRACET( _writeTag, "[{:x}]Client::OnClose", Id() );
		CloseTasks( [](std::any&&){} );
		if( _closeHandle )
			_closeHandle.resume();
		_closeHandle = nullptr;
	}
}