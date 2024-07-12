#pragma once

	//https://www.boost.org/doc/libs/1_73_0/libs/beast/example/http/server/async/http_server_async.cpp
namespace Jde::Web::Rest{
	namespace beast = boost::beast;
	using tcp = boost::asio::ip::tcp;
	struct IRestSession;

	struct ΓW IRestServer : IShutdown{
	  IRestServer( PortType defaultPort )ε;
		virtual ~IRestServer(){}
    α Run()ι->void{ DoAccept(); }
	  α DoAccept()ι->void;
	private:
	  α OnAccept(beast::error_code ec, tcp::socket socket)ι->void;
		β CreateSession( tcp::socket&& socket )ι->sp<IRestSession> =0;
		virtual auto MakeShared()ι->sp<void> =0;
		α Shutdown()ι->void;

		sp<IO::AsioContextThread> _pIOContext;
		tcp::acceptor _acceptor;
	};

	template<class TSession>
	struct RestServer : IRestServer, std::enable_shared_from_this<RestServer<TSession>>{
	  RestServer( PortType port )ε:IRestServer{ port }{}
		virtual ~RestServer(){ DBGT( AppTag(), "~RestServer" ); }
		β CreateSession( tcp::socket&& socket )ι->sp<IRestSession> override;
		α MakeShared()ι->sp<void> override{ return this->shared_from_this(); }
	};

	Ŧ RestServer<T>::CreateSession( tcp::socket&& socket )ι->sp<IRestSession>{
		return ms<T>( move(socket) );
  }
}