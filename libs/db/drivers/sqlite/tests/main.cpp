#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>

#define let const auto

namespace Jde::DB::Sqlite::Schema{
	α Create()ε->void;
}

//The meta graph has pre-existing reference cycles (Column::Table/PKTable are sp<View> back-refs into View::Columns),
//so process-lifetime schema metadata never frees and LeakSanitizer reports it. The real fix (wp<View> back-refs) is
//tracked in reviews/dbReview.md TODO; until then, suppress just those allocation sites so unrelated leaks still surface.
extern "C" const char* __lsan_default_suppressions(){
	return
		"leak:Jde::DB::Sqlite::loadTables\n"
		"leak:Jde::DB::TableDdl\n"
		"leak:Jde::DB::ColumnDdl\n"
		"leak:Jde::DB::Column\n"
		"leak:Jde::DB::View\n"
		"leak:Jde::DB::Table\n"
		"leak:Jde::DB::AppSchema\n"
		"leak:Jde::DB::DBSchema\n"
		"leak:Jde::DB::Catalog\n"
		"leak:Jde::DB::Cluster\n";
}
namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.DB.Sqlite"; }
#endif
	Ω startup( int argc, char **argv )ε->void{
		Process::Startup( argc, argv, Process::ProductName(), "Sqlite driver tests", true );
		Logging::Init();
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	let filterSet = Process::Args().find( "--gtest_filter" )!=Process::Args().end();
	::testing::InitGoogleTest( &argc, argv );
	int exitCode{ EXIT_FAILURE };
	try{
		startup( argc, argv );
		if( !filterSet )
			::testing::GTEST_FLAG( filter ) = Settings::FindSV( "/testing/tests" ).value_or( "*" );
		exitCode = RUN_ALL_TESTS();
	}
	catch( exception& e ){
		if( auto p = dynamic_cast<Exception*>(&e); p ){
			p->Log();
			exitCode = p->HasCode() ? (int)p->Code() : EXIT_FAILURE;
		}
		std::cerr << e.what() << std::endl;
	}
	Process::Shutdown( exitCode );
	return exitCode;
}
