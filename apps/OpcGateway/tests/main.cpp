#include "gtest/gtest.h"
#include <jde/fwk/settings.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/app/client/IAppClient.h>
#include <jde/opc/uatypes/Logger.h>
#include "../src/StartupAwait.h"
#include "../../AppServer/src/AppStartupAwait.h"
#include "../../OpcServer/src/StartupAwait.h"
#include <jde/tests/SpdlogTestListener.h>
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.Opc"; }
#endif
	up<exception> _error;

 	Ω startup( int argc, char **argv, atomic_flag& done )ε->VoidAwait::Task{
		Logging::AddTagParser( mu<Opc::UALogParser>() );
		Process::Startup( argc, argv, "Tests.Opc", "Opc tests", true );
		Opc::Gateway::AppClient()->InitLogging( Opc::Gateway::AppClient() );
		try{
			if( Settings::FindBool("/testing/embeddedAppServer").value_or(true) )
				co_await App::Server::AppStartupAwait{ Settings::AsObject("/http/app") };
			if( Settings::FindBool("/testing/embeddedOpcServer").value_or(true) )
				co_await Opc::Server::StartupAwait{ Settings::AsObject("/http/opcServer"), Settings::AsObject("/credentials/opcServer") };

			co_await Opc::Gateway::StartupAwait{ Settings::AsObject("/http/gateway"), Settings::AsObject("/credentials/gateway") };
			done.test_and_set();
			done.notify_one();
		}
		catch( exception& e ){
			_error = ToUP( move(e) );
			if( auto p = dynamic_cast<Exception*>( _error.get() ); p )
				p->Log();
			done.test_and_set();
			done.notify_one();
		}
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
			std::cerr << "startup error: " << _error->what() << std::endl;//throw *_error slices to std::exception, losing the message.
			throw *_error;
		}
		::testing::GTEST_FLAG( filter ) = Settings::FindString( "/testing/tests" ).value_or( "*" );
		Jde::SpdlogTestListener::Config( ::testing::UnitTest::GetInstance()->listeners() );
		result = RUN_ALL_TESTS();
	}
	catch( exception& e ){
		if( auto p = dynamic_cast<Exception*>( &e ); p )
			p->Log();
		Process::ExitException( move(e) );
	}
	Process::Shutdown( result );

	return result;
}