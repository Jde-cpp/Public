#pragma once
DISABLE_WARNINGS
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/exception/diagnostic_information.hpp>
ENABLE_WARNINGS
#include <jde/App.h>
#include <jde/web/exports.h>
#include "../../../../Framework/source/io/Socket.h"
#include "../../../../Framework/source/io/ProtoUtilities.h"
#include "../../../../Framework/source/threading/Mutex.h"

namespace Jde::Web::Socket{
	namespace beast = boost::beast;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;
	using namespace Jde::IO::Sockets;

	struct ΓW IWebSocketServer /*abstract*/ : IO::Sockets::IServerSocket{
		IWebSocketServer( PortType port )ε;
		~IWebSocketServer(){ _acceptor.close(); TRACET( AppTag(), "~IWebSocketServer - WebSocket"); }
		β CreateSession( IWebSocketServer& server, SessionPK id, tcp::socket&& socket )ι->sp<ISession> =0;
	private:
		α DoAccept()ι->void;
		α OnAccept( beast::error_code ec, tcp::socket socket )ι->void;
		atomic<bool> _shutdown{false};
		sp<IO::AsioContextThread> _pContextThread;
	protected:
		tcp::acceptor _acceptor;
	};

	template<class TFromServer, class TServerSession>
	struct TWebSocketServer /*abstract*/ : IWebSocketServer, IShutdown{
		TWebSocketServer( PortType port )ι: IWebSocketServer{ port } {}
		~TWebSocketServer()=0;
		α Push( IO::Sockets::SessionPK sessionId, TFromServer&& m )ι->void;
		α CreateSession( IWebSocketServer& server, SessionPK id, tcp::socket&& socket )ι->sp<ISession> override{ auto p = make_shared<TServerSession>(server, id, move(socket)); p->Run(); return p; }
		α Shutdown()ι->void override;
		α Find( IO::Sockets::SessionPK id )ι->sp<TServerSession>{
			shared_lock l{ _sessionMutex };
			return _sessions.find(id)==_sessions.end() ? sp<TServerSession>{} : static_pointer_cast<TServerSession>( _sessions.find(id)->second );
		}
	protected:
		atomic<bool> _shutdown{false};
		sp<tcp::acceptor> _pAcceptor;
	};

	template<class TFromServer, class TServerSession>
	TWebSocketServer<TFromServer,TServerSession>::~TWebSocketServer()
	{}

#define $ template<class TFromServer, class TServerSession> auto TWebSocketServer<TFromServer,TServerSession>
	$::Shutdown()ι->void{
		_shutdown = true;
		shared_lock l{_sessionMutex};
		for_each( _sessions.begin(), _sessions.end(), []( auto& pair ){ static_pointer_cast<TServerSession>(pair.second)->Close();} );
		_acceptor.close();
	}
}
#undef $