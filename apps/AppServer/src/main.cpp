#include <jde/framework.h>
#include "AppStartupAwait.h"
#include <jde/framework/process/process.h>
#include <jde/crypto/OpenSsl.h>

namespace Jde{
#ifndef _MSC_VER
	α Process::CompanyName()ι->string{ return "Jde-Cpp"; }
	α Process::ProductName()ι->sv{ return "AppServer"; }
#endif

	α startup( int argc, char** argv )ε->void{
		using namespace Jde::App::Server;
		Logging::Entry::SetGenerator( []( sv text ){ return Crypto::CalcMd5(text); } );
		Process::Startup( argc, argv, "Jde.AppServer", "jde-cpp App Server." );
		auto settings = Settings::FindObject( "/http" );
		BlockVoidAwait( AppStartupAwait{settings ? move(*settings) : jobject{}} );
	}
}

α main( int argc, char** argv )->int{
	using namespace Jde;
	int exitCode;
	try{
		startup( argc, argv );
		exitCode = Process::Pause();
	}
	catch( exception& e ){
		exitCode = Process::ExitException( move(e) );
	}
	Process::Shutdown( exitCode );
	return exitCode;
}