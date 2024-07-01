#include <jde/http/IClientSocketSession.h>

namespace Jde{
	static sp<LogTag> _incomingTag{ Logging::Tag( "client.socket.imcoming" ) };
	static sp<LogTag> _outgoingTag{ Logging::Tag( "client.socket.outgoing" ) };
	α Http::IncomingTag()ι->sp<LogTag>{ return _incomingTag; }
}
#define CHECK_EC if( ec ){ CodeException{ static_cast<std::error_code>(ec) }; return; }
namespace Jde::Http{
	IClientSocketSession::IClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι:
		_resolver{ *ioc },
		_stream{ *ioc, ctx },
		_readTimer{ "", _incomingTag },
		_ioContext{ ioc }
	{}

	α IClientSocketSession::Run( string host, PortType port )ι->void{// Start the asynchronous operation
		_host = host;
		_resolver.async_resolve( host, std::to_string(port), beast::bind_front_handler(&IClientSocketSession::OnResolve, shared_from_this()) );// Look up the domain name
	}

	α IClientSocketSession::OnResolve( beast::error_code ec, tcp::resolver::results_type results )ι->void{
		CHECK_EC
		_stream.OnResolve( results, shared_from_this() );
	}

	α IClientSocketSession::OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type ep )ι->void{
		CHECK_EC
		_stream.OnConnect( ep, _host, shared_from_this() );
	}

	α IClientSocketSession::OnSslHandshake( beast::error_code ec )ι->void{
		CHECK_EC
		_stream.AfterHandshake( _host, shared_from_this() );
	}

	α IClientSocketSession::OnHandshake( beast::error_code ec )ι->void{
		CHECK_EC
		_stream.AsyncRead( shared_from_this() );
	}

	α IClientSocketSession::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		CHECK_EC
	}

	α IClientSocketSession::OnRead( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		CHECK_EC //TODO kill session
		_readTimer.Restart();
		OnReadData( _stream.ReadBuffer() );
		_readTimer.Finish();
		_stream.AsyncRead( shared_from_this() );
	}

	α IClientSocketSession::OnClose( beast::error_code ec )ι->void{
		CHECK_EC
		TRACET( _incomingTag, "OnClose" );
	}
}