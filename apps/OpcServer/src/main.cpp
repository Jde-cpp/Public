#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include <jde/opc/uatypes/Logger.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/app/client/IAppClient.h>
#include "globals.h"
#include "StartupAwait.h"

#define let const auto
std::optional<int> _exitCode;
#ifndef _MSC_VER
	α Jde::Process::ProductName()ι->sv{ return "OpcServer"; }
#endif

α main( int argc, char **argv )->int{
	using namespace Jde;
	Logging::AddTagParser( mu<Opc::UALogParser>() );
	int exitCode;
	try{
		Process::Startup( argc, argv, "Jde.OpcServer", "OpcServer" );
		Opc::Server::AppClient()->InitLogging( Opc::Server::AppClient() );
		let webServerSettings = Settings::FindObject( "/http" );
		BlockVoidAwait( Opc::Server::StartupAwait{webServerSettings ? *webServerSettings : jobject{}, Settings::AsObject("/credentials")} );
		exitCode = Process::Pause();
	}
	catch( exception& e ){
		exitCode = Process::ExitException( move(e) );
	}
	Process::Shutdown( exitCode );
	return exitCode;
}