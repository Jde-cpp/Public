#pragma once
#include <jde/App.h>
#include <jde/web/exports.h>
#include <jde/web/socket/WebSocketServer.h>
#include "../../../../Framework/source/io/Socket.h"
#include "../../../../Framework/source/io/ProtoUtilities.h"
#include "../../../../Framework/source/threading/Mutex.h"


#define var const auto
#define _logTag WebSocketRequestTag()

namespace Jde::Web::Socket{
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

	ΓW α WebSocketRequestTag()ι->sp<Jde::LogTag>;

	struct ΓW IWebSocketSession /*abstract*/: IO::Sockets::ISession, std::enable_shared_from_this<IWebSocketSession>{
		IWebSocketSession( IWebSocketServer& server, SessionPK id, tcp::socket&& socket ):ISession{id}, _ws{std::move(socket)}, _server{server}{ _ws.binary( true ); }
		β Close()ι->void{};
		β Run()ι->void;
	protected:
		α Disconnect( CodeException&& e )ι{ OnDisconnect(move(e)); /*_connected = false;*/ _server.RemoveSession( Id ); }
		β OnDisconnect( CodeException&& )ι->void{}
		β OnAccept( beast::error_code ec )ι->void;

		SocketStream _ws;
		IWebSocketServer& _server;
	private:
		α OnRun()ι->void;
		α DoRead()ι->void;
		β OnRead( const char* p, uint size )ι->void=0;
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )ι->void;

		beast::flat_buffer _buffer;
	};

	template<class TFromServer, class TFromClient>
	struct TWebSocketSession /*abstract*/ : IWebSocketSession{
		TWebSocketSession( IWebSocketServer& server, SessionPK id, tcp::socket&& socket )ε : IWebSocketSession{ server, id, move(socket) }{}

		α OnRead( const char* p, uint size )ι->void;
		β OnRead( TFromClient transmission )ι->void = 0;
		α Write( TFromServer&& message )ε->Task;
		α Write( up<string> data )ι->Task;
		α UserId()Ι{ return _userId; }
	private:
		CoLock _writeLock;
		uint32 _userId{};
	};

#define $ template<class TFromServer, class TFromClient> auto TWebSocketSession<TFromServer,TFromClient>
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
