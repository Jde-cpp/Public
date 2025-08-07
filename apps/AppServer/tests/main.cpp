#include "gtest/gtest.h"
#include "../../Framework/source/Settings.h"
#include <jde/App.h>
#include <jde/app/shared/proto/App.FromClient.pb.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include "../../AppServer/source/LogData.h"
#include "../../Framework/source/db/Database.h"

#define let const auto
namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.Crypto"; }
 	void Startup( int argc, char **argv )ι{
#ifdef _MSC_VER
		ASSERT( Settings::Get<uint>("workers/drive/threads")>0 )
#endif
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		OSApp::Startup( argc, argv, "Tests.Crypto", "Test app" );
		Threading::SetThreadDscrptn( "Main" );
	}
}

int main(int argc, char **argv){
	using namespace Jde;

 	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		let p=Settings::Get<string>( "testing/tests" );
		let filter = p ? *p : "*";
		::testing::GTEST_FLAG( filter ) = filter;
	  result = RUN_ALL_TESTS();
		Process::Shutdown( result );
	}
	return result;
}