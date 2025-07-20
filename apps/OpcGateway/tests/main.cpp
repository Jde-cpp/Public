#include "gtest/gtest.h"
#include <jde/framework/settings.h>
#include "../../../../AppServer/source/AppStartupAwait.h"
#include <jde/framework/thread/execution.h>
#include <jde/framework/coroutine/Timer.h>
#include <jde/db/db.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/opc/uatypes/Logger.h>
#include "../src/StartupAwait.h"
#include "../../OpcServer/src/StartupAwait.h"
#include "helpers.h"
#define let const auto

namespace Jde{
	α Process::ProductName()ι->sv{ return "Tests.Opc"; }
	up<exception> _exception;
	Ω keepExecuterAlive()ι->VoidAwait<>::Task{
		co_await DurationTimer{ 360s };
	}

 	Ω startup( int argc, char **argv, atomic_flag& done )ε->VoidAwait<>::Task{
#ifdef _MSC_VER
		ASSERT( Settings::FindNumber<uint>("/workers/drive/threads").value_or(0)>0 )
#endif
		Logging::AddTagParser( mu<Opc::UALogParser>() );
		OSApp::Startup( argc, argv, "Tests.Opc", "Opc tests", true );
		keepExecuterAlive();
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
			_exception = ToUP( move(e) );
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
		if( _exception )
			throw *_exception;
		::testing::GTEST_FLAG( filter ) = Settings::FindString( "/testing/tests" ).value_or( "*" );
		result = RUN_ALL_TESTS();
	}
	catch( exception& e ){
		Process::ExitException( move(e) );
	}
	Process::Shutdown( result );

	return result;
}