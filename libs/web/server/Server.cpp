#include <jde/web/server/Server.h>
#include <jde/app/IApp.h>
#include <jde/fwk/process/execution.h>
#include <jde/web/usings.h>
#include "ServerImpl.h"
#define let const auto

namespace Jde::Web{
	uint bodyLimit{};
	α Server::BodyLimit()ι->uint{
		if( !bodyLimit )
			bodyLimit = Settings::FindNumber<uint>( "/http/bodyLimit" ).value_or( 10000 );
		return bodyLimit;
	}

	uint socketMessageMax{};
	α Server::SocketMessageMax()ι->uint{
		if( !socketMessageMax )
			//1 MB, not beast's 16 MB default: ql-review3 #16 capped this because the QL parser recursed through a whole
			//16 MB frame on an 8 MB io thread stack.  Two orders of magnitude above the http body cap is enough for a log
			//backlog and still far short of that.
			socketMessageMax = Settings::FindNumber<uint>( "/http/socketMessageMax" ).value_or( 1'000'000 );
		return socketMessageMax;
	}

	static optional<uint16> _maxLogLength;
	α Server::MaxLogLength()ι->uint16{
		if( !_maxLogLength )
			_maxLogLength = Settings::FindNumber<uint16>( "/http/maxLogLength" ).value_or( 1024 );
		return *_maxLogLength;
	}
	α Server::Start( sp<IRequestHandler> handler )ε->void{
		Internal::Start( move(handler) );
	}
	α Server::Stop( sp<IRequestHandler>&& handler, bool terminate, SL sl )ι->void{
		Internal::Stop( move(handler), terminate, sl );
	}
}