#include <jde/web/server/Server.h>
#include <jde/web/server/IApplicationServer.h>
#include <jde/framework/thread/execution.h>
#include "CancellationSignals.h"
#include <jde/web/usings.h>
#include "ServerImpl.h"
#define let const auto

namespace Jde::Web{
	static optional<uint16> _maxLogLength;
	α Server::MaxLogLength()ι->uint16{ 
		if( !_maxLogLength )
			_maxLogLength = Settings::FindNumber<uint16>( "http/maxLogLength" ).value_or( 1024 );
		return *_maxLogLength; 
	}
	α Server::Start( up<IRequestHandler>&& handler, up<IApplicationServer>&& server )ε->void{ Internal::Start( move(handler), move(server) ); }
	α Server::Stop( bool terminate )ι->void{ Internal::Stop(terminate); }
}