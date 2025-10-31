﻿#include <jde/fwk/process/process.h>
#include <jde/opc/uatypes/Logger.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include "StartupAwait.h"

#define let const auto
#ifndef _MSC_VER
	α Jde::Process::ProductName()ι->sv{ return "OpcGateway"; }
#endif

α main( int argc, char **argv )->int{
	using namespace Jde;
	Logging::AddTagParser( mu<Opc::UALogParser>() );
	int exitCode{ EXIT_FAILURE };
	try{
		Process::Startup( argc, argv, "Jde.OpcGateway", "IOT Connection" );
		let webServerSettings = Settings::FindObject("/http");
		let userName = Settings::FindObject("/credentials");
		BlockVoidAwait( Opc::Gateway::StartupAwait{webServerSettings ? *webServerSettings : jobject{}, userName ? *userName : jobject{}} );
		exitCode = Process::Pause();
	}
	catch( exception& e ){
		Jde::Process::ExitException( move(e) );
	}

	Process::Shutdown( exitCode );
	return exitCode;
}