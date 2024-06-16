#include <jde/web/socket/WebSocketServer.h>
#include <jde/Exception.h>

#define var const auto
namespace Jde::Web::Socket{
//	sp<Jde::LogTag> _logTag = WebSocketRequestTag()();
	
	IWebSocketServer::IWebSocketServer( PortType port )ε:
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
			throw CodeException( e.what(), static_cast<std::error_code>(e.code()) );
		}
	}

#define CHECK_EC(ec, ...) if( ec ){ CodeException x(static_cast<std::error_code>(ec) __VA_OPT__(,) __VA_ARGS__); return; }
	α IWebSocketServer::DoAccept()ι->void{
		_acceptor.async_accept( net::make_strand(_pContextThread->Context()), [this]( beast::error_code ec, tcp::socket socket )ι{
				if( ec ){
					ELogLevel level = ec == net::error::operation_aborted ? ELogLevel::Debug : ELogLevel::Error;
					CodeException x{static_cast<std::error_code>(ec), level};
				}
				if( /*ec.value() == 125 &&*/ IApplication::ShuttingDown() ){//125 - Operation canceled
					INFOT( AppTag(), "Websocket shutdown");
					return;
				}
				CHECK_EC( ec );
				var id = ++_id;
				sp<ISession> pSession = CreateSession( *this, id, move(socket) );//deadlock if included in _sessions.emplace
				unique_lock l{ _sessionMutex };
				_sessions.emplace( id, pSession );
				DoAccept();
			} );
	}
}