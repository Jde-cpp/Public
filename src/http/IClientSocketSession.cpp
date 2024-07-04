#include <jde/http/IClientSocketSession.h>

namespace Jde{
	static sp<LogTag> _incomingTag{ Logging::Tag( "client.socket.incoming" ) };
	static sp<LogTag> _outgoingTag{ Logging::Tag( "client.socket.outgoing" ) };
	α Http::IncomingTag()ι->sp<LogTag>{ return _incomingTag; }
}
#define CHECK_EC if( ec ){ CodeException{ static_cast<std::error_code>(ec), GetLogLevel(ec) }; return; }
namespace Jde::Http{
	α GetLogLevel( beast::error_code ec )->ELogLevel{
		return ec == net::error::operation_aborted
			? ELogLevel::Trace
			: ELogLevel::Error;
	}

	α IClientSocketSession::AddTask( RequestId requestId, std::any hCoroutine )ι->void{
		_tasks.emplace( requestId, hCoroutine );
	}
	α IClientSocketSession::GetTask( RequestId requestId )ι->std::any{
		std::any h;
		if( !_tasks.erase_if(requestId, [&h](auto&& kv){ h=kv.second; return true;}) )
			CRITICALT( _incomingTag, "RequestId '{}' not found.", requestId );
		return h;
	}
	α IClientSocketSession::CloseTasks( function<void(std::any&&)> f )ι->void{
		_tasks.erase_if( [ f ](auto&& kv){
			f( move(kv.second) );
			return true;
		});
	}

	CreateClientSocketSessionAwait::CreateClientSocketSessionAwait( sp<IClientSocketSession> session, string host, PortType port, SL sl )ι:
		_session{ session },
		_host{ host },
		_port{ port },
		_sl{ sl }
	{}



	α CreateClientSocketSessionAwait::await_suspend( THandle h )ι->void{
		base::await_suspend( h );
		_session->Run( _host, _port, h );
	}
	α CreateClientSocketSessionAwait::await_resume()ι->void{
	}

	atomic<RequestId> _requestId{ 1 };
	α IClientSocketSession::NextRequestId()ι->RequestId{ return _requestId++; }

	IClientSocketSession::IClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		_resolver{ *ioc },
		_stream{ ms<HttpSocketStream>(*ioc, ctx) },
		_readTimer{ "", _incomingTag },
		_ioContext{ ioc }
	{}

	α IClientSocketSession::Run( string host, PortType port, coroutine_handle<CreateClientSocketSessionAwait::TPromise> h )ι->void{// Start the asynchronous operation
		_connectPromise = h;
		_host = host;
		boost::asio::post( *_ioContext, [&, port_=port, self=shared_from_this()]{
			beast::error_code ec;
			auto results = _resolver.resolve( _host, std::to_string(port_), ec );//async_resolve starts another thread.
			//_resolver.async_resolve( _host, std::to_string(port_), beast::bind_front_handler(&IClientSocketSession::OnResolve, shared_from_this()) );// Look up the domain name
			self->OnResolve( ec, results );
		});
	}

	α IClientSocketSession::OnResolve( beast::error_code ec, tcp::resolver::results_type results )ι->void{
		CHECK_EC
		_stream->OnResolve( results, shared_from_this() );
	}

	α IClientSocketSession::OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type ep )ι->void{
		CHECK_EC
		_stream->OnConnect( ep, _host, shared_from_this() );
	}

	α IClientSocketSession::OnSslHandshake( beast::error_code ec )ι->void{
		CHECK_EC
		_stream->AfterHandshake( _host, shared_from_this() );
	}

	α IClientSocketSession::OnHandshake( beast::error_code ec )ι->void{
		CHECK_EC
		if( _connectPromise )
			_connectPromise.resume();
		_stream->AsyncRead( shared_from_this() );
	}
	α IClientSocketSession::Write( string&& m )ι->void{
		_stream->AsyncWrite( move(m), shared_from_this() );
	}

	// α IClientSocketSession::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
	// 	boost::ignore_unused( bytes_transferred );
	// 	CHECK_EC
	// }

	α IClientSocketSession::OnRead( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		if( ec ){
			Close();
			//OnClose( ec );
			return;
		}
		_readTimer.Restart();//Set on Client.
		OnReadData( _stream->ReadBuffer() );
		_readTimer.Finish();
		_stream->AsyncRead( shared_from_this() );
	}

	α IClientSocketSession::OnClose( beast::error_code ec )ι->void{
		CHECK_EC
		TRACET( _incomingTag, "OnClose" );
	}
}