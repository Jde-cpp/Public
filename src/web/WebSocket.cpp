#include "WebSocket.h"
#include <jde/Exception.h>

#define var const auto
namespace Jde::WebSocket
{
	static const LogTag& _logLevel = Logging::TagLevel( "webRequests" );
	const LogTag& Session::_logLevel = Logging::TagLevel( "net" );;
	WebListener::WebListener( PortType port )ε:
		IServerSocket{ port },
		_pContextThread{ IO::AsioContextThread::Instance() },
		_acceptor{ _pContextThread->Context() }{
		try{
			const tcp::endpoint ep{ boost::asio::ip::tcp::v4(), ISocket::Port };
			_acceptor.open( ep.protocol() );
			_acceptor.set_option( net::socket_base::reuse_address(true) );
			_acceptor.bind( ep );
			_acceptor.listen( net::socket_base::max_listen_connections );
			DoAccept();
		}
		catch( boost::system::system_error& e ){
			throw CodeException( e.what(), e.code() );
		}
	}

#define CHECK_EC(ec, ...) if( ec ){ CodeException x(ec __VA_OPT__(,) __VA_ARGS__); return; }
	α WebListener::DoAccept()ι->void{
		_acceptor.async_accept( net::make_strand(_pContextThread->Context()), [this]( beast::error_code ec, tcp::socket socket )ι{
				if( ec ){
					ELogLevel level = ec == net::error::operation_aborted ? ELogLevel::Debug : ELogLevel::Error;
					CodeException x{ec, level};
				}
				if( /*ec.value() == 125 &&*/ IApplication::ShuttingDown() )//125 - Operation canceled
					return INFO("Websocket shutdown");
				CHECK_EC( ec );
				var id = ++_id;
				sp<ISession> pSession = CreateSession( *this, id, move(socket) );//deadlock if included in _sessions.emplace
				unique_lock l{ _sessionMutex };
				_sessions.emplace( id, pSession );
				DoAccept();
			} );
	}

	α Session::Run()ι->void{
		LOG( "({})Session::Run()", Id );
		net::dispatch( _ws.get_executor(), beast::bind_front_handler(&Session::OnRun, shared_from_this()) );
	}

	α Session::OnRun()ι->void{
		LOG( "({})Session::OnRun()", Id );
		_ws.set_option( websocket::stream_base::timeout::suggested(beast::role_type::server) );
		_ws.set_option( websocket::stream_base::decorator([]( websocket::response_type& res )
			{
				res.set( http::field::server, string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async" );
			}) );
		_ws.async_accept( beast::bind_front_handler(&Session::OnAccept,shared_from_this()) );
	}

	α Session::OnAccept( beast::error_code ec )ι->void{
		LOG( "({})Session::OnAccept()", Id );
		CHECK_EC( ec );
		DoRead();
	}

	α Session::DoRead()ι->void{
		LOG( "({})Session::DoRead()", Id );
		_ws.async_read( _buffer, [this]( beast::error_code ec, uint c )ι{
			boost::ignore_unused( c );
			if( ec ){
				using namespace boost::asio::error;
				var level = ec == websocket::error::closed || ec==not_connected || ec==connection_reset ? LogLevel().Level : ELogLevel::Error;
				Disconnect( CodeException{ec, level} );
				return;
			}
			LOG( "({})Session::DoRead({})", Id, c );
			OnRead( (char*)_buffer.data().data(), _buffer.size() );
			_buffer.clear();
			DoRead();
		} );
	}

	α Session::OnWrite( beast::error_code ec, uint c )ι->void{
		boost::ignore_unused( c );
		try{
			THROW_IFX( ec, CodeException(ec, ec == websocket::error::closed ? LogLevel().Level : ELogLevel::Error) );
		}
		catch( const CodeException& )
		{}
	}
}