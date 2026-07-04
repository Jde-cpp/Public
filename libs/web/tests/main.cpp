#ifdef BOOST_ALL_NO_LIB
#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/process/thread.h>
#include <jde/fwk/log/log.h>
#define let const auto

namespace Jde{
	constexpr ELogTags _tags{ ELogTags::Test };
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.Web"; }
#endif

 	α Startup( int argc, char **argv )ι->void{
		Thread::SetName( "Main" );
		Process::Startup( argc, argv, "Tests.Web", "Web tests", true );
		Logging::Init();
	}

	//Routes GoogleTest's own console output through the shared spdlog console sink instead of
	//writing straight to stdout, so it can't race/tear against log lines written from other
	//threads mid-test (spdlog's stdout_color_sink_mt only serializes writes made through it).
	struct SpdlogTestListener : ::testing::EmptyTestEventListener{
		α OnTestIterationStart( const ::testing::UnitTest& unit, int )->void override{
			INFO( "[==========] Running {} tests from {} test suites.", unit.test_to_run_count(), unit.test_suite_to_run_count() );
		}
		α OnEnvironmentsSetUpStart( const ::testing::UnitTest& )->void override{
			INFO( "[----------] Global test environment set-up." );
		}
		α OnTestSuiteStart( const ::testing::TestSuite& suite )->void override{
			INFO( "[----------] {} tests from {}", suite.test_to_run_count(), suite.name() );
		}
		α OnTestStart( const ::testing::TestInfo& info )->void override{
			INFO( "[ RUN      ] {}.{}", info.test_suite_name(), info.name() );
		}
		α OnTestPartResult( const ::testing::TestPartResult& result )->void override{
			if( result.failed() )
				ERR( "{}({}): {}", result.file_name() ? result.file_name() : "unknown file", result.line_number(), result.message() );
		}
		α OnTestEnd( const ::testing::TestInfo& info )->void override{
			let ms = info.result()->elapsed_time();
			if( info.result()->Passed() ){
				INFO( "[       OK ] {}.{} ({} ms)", info.test_suite_name(), info.name(), ms );
			}else
				ERR( "[  FAILED  ] {}.{} ({} ms)", info.test_suite_name(), info.name(), ms );
		}
		α OnTestSuiteEnd( const ::testing::TestSuite& suite )->void override{
			INFO( "[----------] {} tests from {} ({} ms total)", suite.test_to_run_count(), suite.name(), suite.elapsed_time() );
		}
		α OnEnvironmentsTearDownStart( const ::testing::UnitTest& )->void override{
			INFO( "[----------] Global test environment tear-down" );
		}
		α OnTestIterationEnd( const ::testing::UnitTest& unit, int )->void override{
			INFO( "[==========] {} tests from {} test suites ran. ({} ms total)", unit.test_to_run_count(), unit.test_suite_to_run_count(), unit.elapsed_time() );
			INFO( "[  PASSED  ] {} tests.", unit.successful_test_count() );
			if( let failed = unit.failed_test_count() ){
				ERR( "[  FAILED  ] {} tests, listed below:", failed );
				for( auto i=0; i<unit.total_test_suite_count(); ++i ){
					let& suite = *unit.GetTestSuite( i );
					for( auto j=0; j<suite.total_test_count(); ++j ){
						let& test = *suite.GetTestInfo( j );
						if( test.result()->Failed() )
							ERR( "[  FAILED  ] {}.{}", suite.name(), test.name() );
					}
				}
			}
		}
	};
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		let filter=Settings::FindSV( "/testing/tests" ).value_or( "*" );
		::testing::GTEST_FLAG( filter ) = filter;
		auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
		delete listeners.Release( listeners.default_result_printer() );
		listeners.Append( new SpdlogTestListener{} );
	   result = RUN_ALL_TESTS();
		Process::Shutdown( result );
	}
	return result;
}