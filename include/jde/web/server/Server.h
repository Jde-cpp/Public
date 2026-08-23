#pragma once
#include "Sessions.h"

#define Φ ΓWS auto
namespace Jde::Web::Server{
	struct IRequestHandler;
	α BodyLimit()ι->uint;
	α SocketMessageMax()ι->uint;
	α MaxLogLength()ι->uint16;
	Φ Start( sp<IRequestHandler> handler )ε->void;
	Φ Stop( sp<IRequestHandler>&& handler, bool terminate=false, SRCE )ι->void;
}
#undef Φ