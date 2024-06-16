#include <jde/web/socket/_ISocketSession.h>

#define var const auto
namespace Jde::IO::Sockets{
	static sp<Jde::LogTag> _logTag{ Logging::Tag("TcpSession") };
	α ISocketSession::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }
	ISocketSession::ISocketSession( tcp::socket&& socket, SessionPK id )ι:
		Id{ id },
		_socket( move(socket) ){
		ReadHeader();
	}

	α ISocketSession::ReadHeader()ι->void{
		net::async_read( _socket, net::buffer(static_cast<void*>(_readMessageSize), sizeof(_readMessageSize)), [&]( std::error_code ec, uint headerLength )ι{
			try{
				THROW_IFX( ec && ec.value()==2, CodeException(format("[{}] - Disconnected", Id), move(ec), ELogLevel::Trace) );
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

	α ISocketSession::Write( up<google::protobuf::uint8[]> p, uint c )ι->void{
		auto buf = net::buffer( p.get(), c );
		net::async_write( _socket, buf, [_=move(p), _1=move(buf), _2=shared_from_this()]( std::error_code ec, std::size_t length ){
			if( ec )
				DBGT( LogTag(), "[{}]Write message returned '{}'.", ec.value(), ec.message() );
			else
				TRACET( LogTag(), "Session::Write length:  '{}'.", length );
		});
	}
}