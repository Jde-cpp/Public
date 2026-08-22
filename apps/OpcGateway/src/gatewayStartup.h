#pragma once
#include <jde/fwk/usings.h>

namespace Jde::Opc::Gateway{
	//Blocks until the gateway is up: schema sync, web server, AppServer connect, then access configure.  Call from a thread
	//that does not run the io pool (main) - the connect and access steps block on awaitables.
	α Startup( jobject webServerSettings, jobject userName )ε->void;
}