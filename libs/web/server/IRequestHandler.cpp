#include <jde/web/server/IRequestHandler.h>

namespace Jde::Web::Server{

	IRequestHandler::IRequestHandler( jobject settings, sp<App::IApp> appServer )ι:
		_appServer{move(appServer)},
		_cancelSignal{ ms<net::cancellation_signal>() },
		_ctx{ ssl::context{ssl::context::tlsv12} },
		_settings{settings}
	{}

	α IRequestHandler::BlockTillStarted()ι->void{
		if( !_started.test_and_set() )
			_started.wait( false );
	}
	α IRequestHandler::Start()ι->void{
		_started.test_and_set();
		_started.notify_all();
	}
	α IRequestHandler::Stop()ι->void{
		if( _cancelSignal )
			_cancelSignal->emit( net::cancellation_type::all );
//		_cancelSignal = nullptr; heap use after free.
		_started.clear();
		_started.notify_all();
	}
}