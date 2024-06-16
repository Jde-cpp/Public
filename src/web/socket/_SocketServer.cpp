#include <jde/web/socket/_SocketServer.h>
#include "../../../../Framework/source/io/AsioContextThread.h"

#define var const auto
namespace Jde::IO::Sockets{
	static sp<Jde::LogTag> _logTag{ Logging::Tag("webSocket") };
	
	SocketServer::SocketServer( PortType port )ι:
		ISocket{ port },
		_pIOContext{ IO::AsioContextThread::Instance() },
		_acceptor{ _pIOContext->Context(), tcp::endpoint{ Settings::Get("net/ip").value_or("v6")=="v6" ? tcp::v6() : tcp::v4(), port } }
	{}

	SocketServer::~SocketServer()
	{}

	α SocketServer::Accept()ι->void{
		_acceptor.async_accept( [this]( std::error_code ec, tcp::socket socket )ι{
			try{
				THROW_IFX( ec, CodeException(ec.value()==125 ? "Sever shutting down" : "Accept Failed", move(ec), ec.value()==125 ? ELogLevel::Information : ELogLevel::Error) );
				var id = ++_id;
				DBG( "[{}]Accepted Connection", id );
				{
					ul _{ _mutex };
					auto p = CreateSession( move(socket), id );
					_sessions.emplace( id, p );
				}
				Accept();
			}
			catch( const IException& ){}
		});
	}
}