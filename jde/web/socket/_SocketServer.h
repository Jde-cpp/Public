#pragma once
#include <jde/Exports.h>
#include <jde/web/exports.h>
#include "google/protobuf/message.h"
#include "../../../../Framework/source/io/Socket.h"
#include "../../../../Framework/source/io/AsioContextThread.h"

#define var const auto

namespace Jde::IO::Sockets{
	struct ISocketSession;
	struct SocketServer : ISocket{
		ΓW SocketServer( PortType defaultPort )ι;
		ΓW virtual ~SocketServer();
		β CreateSession( tcp::socket&& socket, SessionPK id )ι->sp<ISocketSession> =0;
		α RemoveSession( SessionPK id )ι{ ul _{_mutex}; _sessions.erase(id); }

	protected:
		ΓW auto Accept()ι->void;
		std::atomic<SessionPK> _id{0};
		sp<IO::AsioContextThread> _pIOContext;
		tcp::acceptor _acceptor;
		flat_map<SessionPK,sp<ISocketSession>> _sessions; mutable std::shared_mutex _mutex;
	private:
		void Run()ι;
	};
}