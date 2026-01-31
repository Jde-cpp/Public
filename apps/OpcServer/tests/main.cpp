#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/settings.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/opc/uatypes/Logger.h>
#include "../src/StartupAwait.h"
#include "../../AppServer/src/AppStartupAwait.h"
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.OpcServer"; }
#endif
	up<exception> _error;

 	Ω startup( int argc, char **argv, atomic_flag& done )ε->VoidAwait::Task{
		Logging::AddTagParser( mu<Opc::UALogParser>() );
		Process::Startup( argc, argv, "Tests.OpcServer", "OpcServer tests", true );
		Opc::Server::AppClient()->InitLogging( Opc::Server::AppClient() );
		try{
			if( Settings::FindBool("/testing/embeddedAppServer").value_or(true) )
				co_await App::Server::AppStartupAwait{ Settings::AsObject("/http/app") };
			co_await Opc::Server::StartupAwait{ Settings::AsObject("/http/opcServer"), Settings::AsObject("/credentials/opcServer") };
		}
		catch( exception& e ){
			auto p = ToUP( move(e) );
			_error = move(p);
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
		if( _error ){
			Throw( move(*_error) );
		}
		::testing::GTEST_FLAG( filter ) = Settings::FindString( "/testing/tests" ).value_or( "*" );
		result = RUN_ALL_TESTS();
	}
	catch( exception& e ){
		Process::ExitException( move(e) );
	}
	Process::Shutdown( result );

	return result;
}