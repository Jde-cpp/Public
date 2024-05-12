#pragma once
DISABLE_WARNINGS
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/exception/diagnostic_information.hpp>
ENABLE_WARNINGS
#include <jde/App.h>
#include "../../../Framework/source/io/Socket.h"
#include "../../../Framework/source/io/ProtoUtilities.h"
#include "../../../Framework/source/threading/Mutex.h"
#include "Exports.h"

#define var const auto
#define _logTag WebSocketTag()
namespace Jde::WebSocket
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	using namespace Jde::IO::Sockets;
#ifdef HTTPS
	typedef websocket::stream<beast::ssl_stream<beast::tcp_stream>> SocketStream;
#else
	typedef websocket::stream<beast::tcp_stream> SocketStream;
#endif
	ΓW α WebSocketTag()ι->sp<Jde::LogTag>;

	struct ΓW WebListener /*abstract*/ : IO::Sockets::IServerSocket{
		WebListener( PortType port )ε;
		~WebListener(){ _acceptor.close(); TRACE("~WebListener - WebSocket"); }
		β CreateSession( WebListener& server, SessionPK id, tcp::socket&& socket )ι->sp<ISession> =0;
	private:
		α DoAccept()ι->void;
		α OnAccept( beast::error_code ec, tcp::socket socket )ι->void;
		atomic<bool> _shutdown{false};
		sp<IO::AsioContextThread> _pContextThread;
	protected:
		tcp::acceptor _acceptor;
	};

	template<class TFromServer, class TServerSession>
	struct TListener /*abstract*/ : WebListener, IShutdown{
		TListener( PortType port )ι: WebListener{ port } {}
		~TListener()=0;
		α Push( IO::Sockets::SessionPK sessionId, TFromServer&& m )ι->void;
		α CreateSession( WebListener& server, SessionPK id, tcp::socket&& socket )ι->sp<ISession> override{ auto p = make_shared<TServerSession>(server, id, move(socket)); p->Run(); return p; }
		α Shutdown()ι->void override;
		α Find( IO::Sockets::SessionPK id )ι->sp<TServerSession>{
			shared_lock l{ _sessionMutex };
			return _sessions.find(id)==_sessions.end() ? sp<TServerSession>{} : static_pointer_cast<TServerSession>( _sessions.find(id)->second );
		}
	protected:
		atomic<bool> _shutdown{false};
		sp<tcp::acceptor> _pAcceptor;
	};

	struct ΓW Session /*abstract*/: IO::Sockets::ISession, std::enable_shared_from_this<Session>{
		Session( WebListener& server, SessionPK id, tcp::socket&& socket ):ISession{id}, _ws{std::move(socket)}, _server{server}{ _ws.binary( true ); }
		β Close()ι->void{};
		β Run()ι->void;
	protected:
		α Disconnect( CodeException&& e )ι{ OnDisconnect(move(e)); /*_connected = false;*/ _server.RemoveSession( Id ); }
		β OnDisconnect( CodeException&& )ι->void{}
		β OnAccept( beast::error_code ec )ι->void;

		SocketStream _ws;
		WebListener& _server;
	private:
		α OnRun()ι->void;
		α DoRead()ι->void;
		β OnRead( const char* p, uint size )ι->void=0;
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void;

		beast::flat_buffer _buffer;
	};
	template<class TFromServer, class TFromClient>
	struct TSession /*abstract*/ : Session{//, public std::enable_shared_from_this<TSession<TFromServer,TFromClient>>
		TSession( WebListener& server, SessionPK id, tcp::socket&& socket )ε : Session{ server, id, move(socket) }{}

		α OnRead( const char* p, uint size )ι->void;
		β OnRead( TFromClient transmission )ι->void = 0;
		α Write( TFromServer&& message )ε->Task;
		α Write( up<string> data )ι->Task;
		α UserId()Ι{ return _userId; }
	private:
		CoLock _writeLock;
		uint32 _userId{};
	};

	template<class TFromServer, class TServerSession>
	TListener<TFromServer,TServerSession>::~TListener()
	{}

#define $ template<class TFromServer, class TServerSession> auto TListener<TFromServer,TServerSession>
	$::Shutdown()ι->void{
		_shutdown = true;
		shared_lock l{_sessionMutex};
		for_each( _sessions.begin(), _sessions.end(), []( auto& pair ){ static_pointer_cast<TServerSession>(pair.second)->Close();} );
		_acceptor.close();
	}

#undef $
#define $ template<class TFromServer, class TFromClient> auto TSession<TFromServer,TFromClient>
	$::OnRead( const char* p, uint size )ι->void{
		//DBG( "p='{:x}', size={}", (uint)p, size );
		try{
			auto t = IO::Proto::Deserialize<TFromClient>( (const google::protobuf::uint8*)p, (int)size );
			OnRead( move(t) );
		}
		catch( const IException& )
		{}
	}
	$::Write( TFromServer&& message )ε->Task{
		return Write( mu<string>(IO::Proto::ToString(message)) );
	}
	$::Write( up<string> pData )ι->Task{
		var buffer = net::buffer( (const void*)pData->data(), pData->size() );
		LockAwait await = _writeLock.Lock(); //gcc doesn't like co_await _writeLock.Lock();
		AwaitResult task = co_await await;
		auto lock = task.UP<CoGuard>();
		_ws.async_write( buffer, [ this, pKeepAlive=shared_from_this(), buffer, l=move(lock), p=move(pData) ]( beast::error_code ec, uint bytes_transferred )mutable{
			l = nullptr;
			if( ec || p->size()!=bytes_transferred ){
				Logging::LogNoServer( Logging::Message(ELogLevel::Debug, "Error writing to Session:  '{}'"), _logTag, boost::diagnostic_information(ec) );
				try{
					_ws.close( websocket::close_code::none );
				}
				catch( const boost::exception& e2 ){
					Logging::LogNoServer( Logging::Message{ELogLevel::Debug, "Error closing:  '{}')"}, _logTag, boost::diagnostic_information(e2) );
				}
				Disconnect( CodeException{move(static_cast<std::error_code>(ec))} );
			}
		} );
	}
}
#undef $
#undef var
#undef _logTag
