#include <jde/framework/process.h>
#include <jde/opc/uatypes/Logger.h>
#include "StartupAwait.h"

#define let const auto
α Jde::Process::ProductName()ι->sv{ return "OpcGateway"; }

α main( int argc, char **argv )->int{
	using namespace Jde;
	Logging::AddTagParser( mu<Opc::UALogParser>() );
	int exitCode{ EXIT_FAILURE };
	try{
		OSApp::Startup( argc, argv, "Jde.OpcGateway", "IOT Connection" );
		let webServerSettings = Settings::FindObject("/http");
		BlockVoidAwait( Opc::Gateway::StartupAwait{webServerSettings ? *webServerSettings : jobject{}} );
		exitCode = Process::Pause();
	}
	catch( exception& e ){
		Jde::Process::ExitException( move(e) );
	}

	Process::Shutdown( exitCode );
	return exitCode;
}