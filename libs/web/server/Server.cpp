#include <jde/web/server/Server.h>
#include <jde/app/IApp.h>
#include <jde/framework/process/execution.h>
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
	α Server::Start( sp<IRequestHandler> handler )ε->void{
		Internal::Start( move(handler) );
	}
	α Server::Stop( sp<IRequestHandler>&& handler, bool terminate )ι->void{
		Internal::Stop( move(handler), terminate );
	}
}