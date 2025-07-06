#include <jde/opc/uatypes/Logger.h>
#include "StartupAwait.h"

#define let const auto
std::optional<int> _exitCode;
α Jde::Process::ProductName()ι->sv{ return "OpcServer"; }

α main( int argc, char **argv )->int{
	using namespace Jde;
	Logging::AddTagParser( mu<Opc::UALogParser>() );
	int exitCode;
	try{
		OSApp::Startup( argc, argv, "Jde.OpcServer", "OpcServer" );
		let webServerSettings = Settings::FindObject("/http");
		BlockVoidAwait( Opc::Server::StartupAwait{webServerSettings ? *webServerSettings : jobject{}} );
		exitCode = Process::Pause();
	}
	catch( IException& e ){
		e.Log();
		BREAK;
		exitCode = e.Code ? (int)e.Code : EXIT_FAILURE;
	}
	Process::Shutdown( exitCode );
	return exitCode;
}