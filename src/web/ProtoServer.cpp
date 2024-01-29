#include "ProtoServer.h"

#define var const auto
namespace Jde::IO::Sockets
{
	sp<Jde::LogTag> _logTag{ Logging::Tag("session") };
	α ProtoSession::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }
	ProtoServer::ProtoServer( PortType port )ι:
		ISocket{ port },
		_pIOContext{ AsioContextThread::Instance() },
		_acceptor{ _pIOContext->Context(), tcp::endpoint{ Settings::Get("net/ip").value_or("v6")=="v6" ? tcp::v6() : tcp::v4(), port } }
	{}

	ProtoServer::~ProtoServer()
	{}

	α ProtoServer::Accept()ι->void{
		_acceptor.async_accept( [this]( std::error_code ec, tcp::socket socket )ι{
			try{
				THROW_IFX( ec, CodeException(ec.value()==125 ? "Sever shutting down" : "Accept Failed", move(ec), ec.value()==125 ? ELogLevel::Information : ELogLevel::Error) );
				var id = ++_id;
				DBG( "({})Accepted Connection", id );
				ul _{ _mutex };
				_sessions.emplace( id, CreateSession(move(socket), id) );
				Accept();
			}
			catch( const IException& ){}
		});
	}

	ProtoSession::ProtoSession( tcp::socket&& socket, SessionPK id )ι:
		Id{ id },
		_socket( move(socket) ){
		ReadHeader();
	}

	α ProtoSession::ReadHeader()ι->void{
		net::async_read( _socket, net::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [&]( std::error_code ec, uint headerLength )ι{
			try{
				THROW_IFX( ec && ec.value()==2, CodeException(fmt::format("({}) - Disconnected", Id), move(ec), ELogLevel::Trace) );
				THROW_IFX( ec, CodeException(move(ec), ec.value()==10054 || ec.value()==104 ? ELogLevel::Trace : ELogLevel::Error) );
				THROW_IF( headerLength!=4, "only read '{}'"sv, headerLength );

				var messageLength = ProtoClientSession::MessageLength( _readMessageSize );
				ReadBody( messageLength );
			}
			catch( const IException& e ){
				if( e.Level()<ELogLevel::Error )
					OnDisconnect();
			}
		});
	}

	α ProtoSession::Write( up<google::protobuf::uint8[]> p, uint c )ι->void{
		auto b = net::buffer( p.get(), c );
		net::async_write( _socket, b, [_=move(p), b2=move(b)]( std::error_code ec, std::size_t length ){
			if( ec )
				DBG( "({})Write message returned '{}'.", ec.value(), ec.message() );
			else
				TRACE( "Session::Write length:  '{}'.", length );
		});
	}
}