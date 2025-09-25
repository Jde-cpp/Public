#include "gtest/gtest.h"
#include <jde/framework/settings.h>
#include <jde/framework/coroutine/Timer.h>
#include <jde/crypto/OpenSsl.h>
#include <jde/opc/uatypes/Logger.h>
#include "../src/StartupAwait.h"
#include "../../AppServer/src/AppStartupAwait.h"
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.OpcServer"; }
#endif
	up<exception> _error;
	Ω keepExecuterAlive()ι->VoidAwait::Task{
		co_await DurationTimer{ 360s };
	}

 	Ω startup( int argc, char **argv, atomic_flag& done )ε->VoidAwait::Task{
		Logging::Entry::SetGenerator( []( sv text ){ return Crypto::CalcMd5(text); } );
		Logging::AddTagParser( mu<Opc::UALogParser>() );
		OSApp::Startup( argc, argv, "Tests.OpcServer", "OpcServer tests", true );
		keepExecuterAlive();
		try{
			if( Settings::FindBool("/testing/embeddedAppServer").value_or(true) )
				co_await App::Server::AppStartupAwait{ Settings::AsObject("/http/app") };
			co_await Opc::Server::StartupAwait{ Settings::AsObject("/http/opcServer"), Settings::AsObject("/credentials/opcServer") };
		}
		catch( exception& e ){
			_error = ToUP( move(e) );
		}
		done.test_and_set();
		done.notify_one();
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	atomic_flag done;
	startup( argc, argv, done );
	done.wait( false );
	int result{ EXIT_FAILURE };
	try{
		if( _error )
			throw *_error;
		::testing::GTEST_FLAG( filter ) = Settings::FindString( "/testing/tests" ).value_or( "*" );
		result = RUN_ALL_TESTS();
	}
	catch( exception& e ){
		Process::ExitException( move(e) );
	}
	Process::Shutdown( result );

	return result;
}