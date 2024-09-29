#pragma once
#include "Sessions.h"

#define Φ ΓWS auto
namespace Jde::Web::Server{
	struct IRequestHandler; struct IApplicationServer;
	α MaxLogLength()ι->uint16;
	Φ Start( up<IRequestHandler>&& handler, up<Server::IApplicationServer>&& server )ε->void;
	Φ Stop( bool terminate=false )ι->void;
}
#undef Φ