#include "AppLog.h"
#include "AppClientSocketSession.h"
#include <jde/appClient/AppClient.h>

namespace Jde::App::Client{
	using namespace Jde::Logging;
	α AppLog::Destroy( SL sl )ι->void{ CloseSocketSession(); }
	α AppLog::Log( ExternalMessage&& m, SL sl )ι->void{
		[&]->LogAwait::Task { co_await LogAwait{ move(m), sl }; }();
	}
	α AppLog::Log( const ExternalMessage& m, const vector<string>* args, SL sl )ι->void{
		[&]->LogAwait::Task { co_await LogAwait{ m, args, sl }; }();
	}
	α AppLog::SetMinLevel(ELogLevel level)ι->void{
		UpdateStatus();
		_minLevel = level;
	}
}
// client requires http.
// web requires http.
// Web requires client.
