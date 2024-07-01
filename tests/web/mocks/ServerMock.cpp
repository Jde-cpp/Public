#include "ServerMock.h"

namespace Jde::Web{
	optional<std::jthread> _webThread;

	α Mock::Start()ι->void{
		_webThread = std::jthread{ []{
			Flex::Start( ms<RequestHandler>() ); //blocking
		} };
		while( !Flex::HasStarted() )
			std::this_thread::sleep_for( 100ms );
	}

	α Mock::Stop()ι->void{
		Flex::Stop();
		if( _webThread && _webThread->joinable() ){
			_webThread->join();
			_webThread = {};
		}
	}

}
