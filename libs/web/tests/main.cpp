#ifdef BOOST_ALL_NO_LIB
#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/process/thread.h>
#include <jde/fwk/log/log.h>
#include <jde/tests/SpdlogTestListener.h>
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.Web"; }
#endif

 	α Startup( int argc, char **argv )ι->void{
		Thread::SetName( "Main" );
		Process::Startup( argc, argv, "Tests.Web", "Web tests", true );
		Logging::Init();
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		let filter=Settings::FindSV( "/testing/tests" ).value_or( "*" );
		::testing::GTEST_FLAG( filter ) = filter;
		Jde::SpdlogTestListener::Config( ::testing::UnitTest::GetInstance()->listeners() );
	   result = RUN_ALL_TESTS();
		Process::Shutdown( result );
	}
	return result;
}