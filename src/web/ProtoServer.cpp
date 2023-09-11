#include "ProtoServer.h"
//#include "../../threading/Thread.h"
//#include "../../threading/InterruptibleThread.h"

#define var const auto
namespace Jde::IO::Sockets
{
	const LogTag& ProtoSession::_logLevel{ Logging::TagLevel("session") };

	ProtoServer::ProtoServer( PortType port )noexcept:
		ISocket{ port },
		_pIOContext{ AsioContextThread::Instance() },
		_acceptor{ _pIOContext->Context(), tcp::endpoint{ Settings::Get("net/ip").value_or("v6")=="v6" ? tcp::v6() : tcp::v4(), port } }
	{}

	ProtoServer::~ProtoServer()
	{}

	α ProtoServer::Accept()noexcept->void
	{
		_acceptor.async_accept( [this]( std::error_code ec, tcp::socket socket )noexcept
		{
			try
			{
				THROW_IFX( ec, CodeException(ec.value()==125 ? "Sever shutting down" : "Accept Failed", move(ec), ec.value()==125 ? ELogLevel::Information : ELogLevel::Error) );
				var id = ++_id;
				DBG( "({})Accepted Connection"sv, id );
				unique_lock l{ _mutex };
				_sessions.emplace( id, CreateSession(std::move(socket), id) );
				Accept();
			}
			catch( const IException& ){}
		});
	}

	ProtoSession::ProtoSession( tcp::socket&& socket, SessionPK id )noexcept:
		Id{ id },
		_socket( std::move(socket) )
	{
		ReadHeader();
	}

	α ProtoSession::ReadHeader()noexcept->void
	{
		net::async_read( _socket, net::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [&]( std::error_code ec, uint headerLength )
		{
			try
			{
				THROW_IFX( ec && ec.value()==2, CodeException(fmt::format("({}) - Disconnected", Id), move(ec), _logLevel.Level) );
				THROW_IFX( ec, CodeException(move(ec), ec.value()==10054 || ec.value()==104 ? _logLevel.Level : ELogLevel::Error) );
				THROW_IF( headerLength!=4, "only read '{}'"sv, headerLength );

				var messageLength = ProtoClientSession::MessageLength( _readMessageSize );
				ReadBody( messageLength );
			}
			catch( const IException& e )
			{
				if( e.Level()<ELogLevel::Error )
					OnDisconnect();
			}
		});
	}

	α ProtoSession::Write( up<google::protobuf::uint8[]> p, uint c )noexcept->void
	{
		auto b = net::buffer( p.get(), c );
		net::async_write( _socket, b, [_=move(p), b2=move(b)]( std::error_code ec, std::size_t length )
		{
			if( ec )
				DBG( "({})Write message returned '{}'."sv, ec.value(), ec.message() );
			else
				LOGL( ELogLevel::Trace, "Session::Write length:  '{}'."sv, length );
		});
	}
}