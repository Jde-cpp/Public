#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/settings.h>
#include <jde/tests/SpdlogTestListener.h>
#include <jde/tests/testMain.h>
#include "../src/AppStartupAwait.h"
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.AppServer"; }
#endif
	up<std::exception> _error;

 	Ω startup( int argc, char **argv, atomic_flag& done )ε->VoidAwait::Task{
		Process::Startup( argc, argv, "Tests.AppServer", "AppServer unit tests", true );
		App::Server::InitLogging();
		try{
			co_await App::Server::AppStartupAwait{ Settings::AsObject("/http/app") };
		}
		catch( runtime_error& e ){
			_error = ToUP( move(e) );
			if( auto p = dynamic_cast<Exception*>( _error.get() ); p )
				p->Log();
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
			std::cerr << "startup error: " << _error->what() << std::endl;//throw *_error slices to std::exception, losing the message.
			throw *_error;
		}
		::testing::GTEST_FLAG( filter ) = Settings::FindString( "/testing/tests" ).value_or( "*" );
		Jde::SpdlogTestListener::Config( ::testing::UnitTest::GetInstance()->listeners() );
		result = CheckTestsRan( RUN_ALL_TESTS() );
	}
	catch( std::exception& e ){
		Process::ExitException( move(e) );
	}
	Process::Shutdown( result );

	return result;
}
