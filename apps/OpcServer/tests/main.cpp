#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/settings.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/io/json.h>
#include <jde/opc/uatypes/Logger.h>
#include <jde/tests/SpdlogTestListener.h>
#include <jde/tests/testMain.h>
#include "../src/StartupAwait.h"
#include "../../AppServer/src/AppStartupAwait.h"
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.OpcServer"; }
#endif
	up<std::exception> _error;

 	Ω startup( int argc, char **argv, atomic_flag& done )ε->VoidAwait::Task{
		Logging::AddTagParser( mu<Opc::UALogParser>() );
		Process::Startup( argc, argv, "Tests.OpcServer", "OpcServer tests", true );
		Opc::Server::AppClient()->InitLogging( Opc::Server::AppClient() );
		try{
			if( Settings::FindBool("/testing/embeddedAppServer").value_or(true) )//the fresh db enrolls the client cert every run: /access/trustedCertDirs anchors its dir, StartupAwait ensures the cert, and TrustVerify rescans - no pre-anchoring of the CLIENT cert here.  The other direction (client trusts each embedded server's cert) is covered by Web::Server::Start's self-anchor.
				co_await App::Server::AppStartupAwait{ Settings::AsObject("/http/app") };
			co_await Opc::Server::StartupAwait{ Settings::AsObject("/http/opcServer"), Settings::AsObject("/credentials/opcServer") };
		}
		catch( runtime_error& e ){
			auto p = ToExceptionPtr( move(e) );
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