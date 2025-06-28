#include <iostream>
#include <signal.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <jde/opc/uatypes/Logger.h>
#include <jde/opc/uatypes/Node.h>
#include "usings.h"
#include "globals.h"
#include "startup.h"

#define let const auto
std::optional<int> _exitCode;
α Jde::Process::ProductName()ι->sv{ return "OpcServer"; }
α main( int argc, char **argv )->int{
	using namespace Jde;
	Logging::AddTagParser( mu<Opc::UALogParser>() );
	OSApp::Startup( argc, argv, "Jde.OpcServer", "IOT Connection" );
	Opc::Server::Startup( _exitCode );
	if( !_exitCode )
		_exitCode = Process::Pause();
	Process::Shutdown( _exitCode.value_or(EXIT_FAILURE) );
	return _exitCode.value_or( EXIT_FAILURE );
}
