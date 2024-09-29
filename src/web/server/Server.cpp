#include <jde/web/server/Server.h>
#include <jde/web/server/IApplicationServer.h>
#include <jde/thread/Execution.h>
#include "CancellationSignals.h"
#include <jde/web/usings.h>
#include "ServerImpl.h"
#define var const auto

namespace Jde::Web{
	static uint16 _maxLogLength{ Settings::Get<uint16>("http/maxLogLength").value_or(1024) };
	α Server::MaxLogLength()ι->uint16{ return _maxLogLength; }
	α Server::Start( up<IRequestHandler>&& handler, up<IApplicationServer>&& server )ε->void{ Internal::Start( move(handler), move(server) ); }
	α Server::Stop( bool /*terminate*/ )ι->void{ Internal::Stop(terminate); }
}