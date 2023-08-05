#include <fstream>
/*
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
*/
#include "gtest/gtest.h"
#include <jde/App.h>
#include <jde/io/File.h>
#define var const auto

namespace Jde
{
	α OSApp::ProductName()ι->sv{ return "ML"; }

 	void Startup( int argc, char **argv )noexcept
	{
		var appName = "Tests.ML>"sv;
		OSApp::Startup( argc, argv, appName, "ML Tests" );
	}
}

int main( int argc, char **argv )
{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	var pFilter = Settings::Get<string>( "testing/tests" );
	::testing::GTEST_FLAG( filter ) = pFilter ? *pFilter : "*";
	auto result = RUN_ALL_TESTS();

	IApplication::Cleanup();
	return result;
}