#include <jde/web/rest/RestServer.h>
#include <jde/web/rest/IRestSession.h>

namespace Jde::Web::Rest{
	α RequestTag()ι->sp<LogTag>;
	α ResponseTag()ι->sp<LogTag>;

	IRestServer::IRestServer( PortType port )ε:
		_pIOContext{ IO::AsioContextThread::Instance() },
		_acceptor{ _pIOContext->Context(), tcp::endpoint{ Settings::Get("net/ip").value_or("v6")=="v6" ? tcp::v6() : tcp::v4(), port } }{
		beast::error_code ec;
		INFOT( AppTag(), "Rest listening on port={}", port );
    _acceptor.listen( net::socket_base::max_listen_connections, ec ); THROW_IF( ec, "listen" );
  }

  α IRestServer::DoAccept()ι->void{
		sp<IRestServer> sp = static_pointer_cast<IRestServer>( MakeShared() );
		_acceptor.async_accept( net::make_strand(_pIOContext->Context()), beast::bind_front_handler(&IRestServer::OnAccept, move(sp)) );
	}

  α IRestServer::OnAccept( beast::error_code ec, tcp::socket socket )ι->void{
		TRACET( RequestTag(), "ISession::OnAccept()" );
		if( ec ){
			const ELogLevel level{ ec == net::error::operation_aborted ? ELogLevel::Debug : ELogLevel::Error };
			CodeException{ static_cast<std::error_code>(ec), level };
		}
		else{
			CreateSession( move(socket) )->Run();
    	DoAccept();
		}
  }

	α IRestServer::Shutdown()ι->void{
		_acceptor.close();
		auto spThis = MakeShared();
		DBGT( AppTag(), "Rest::Shutdown use_count={}", spThis.use_count()-1 );//acceptor, global, shutdown
		spThis=nullptr;
	}
}
