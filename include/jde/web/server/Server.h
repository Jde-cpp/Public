#pragma once
#include "Sessions.h"

#define Φ ΓWS auto
namespace Jde::Web::Server{
	struct IRequestHandler;
	α MaxLogLength()ι->uint16;
	Φ Start( sp<IRequestHandler> handler )ε->void;
	Φ Stop( sp<IRequestHandler>&& handler, bool terminate=false )ι->void;
}
#undef Φ