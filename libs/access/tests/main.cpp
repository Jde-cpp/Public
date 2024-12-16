#include "gtest/gtest.h"
#include <jde/framework/process.h>
#include <jde/framework/settings.h>
#include <jde/access/access.h>
#include <jde/db/db.h>
#include <jde/ql/ql.h>
#include <jde/ql/QLHook.h>
#include "globals.h"

#define let const auto

namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.UM"; }

 	α Startup( int argc, char **argv, bool& set )ε->Access::ConfigureAwait::Task{
		try{
	#ifdef _MSC_VER
			ASSERT( Settings::Get<uint>("/workers/drive/threads")>0 )
	#endif
			ASSERT( argc>1 && string{argv[1]}=="-c" )
			OSApp::Startup( argc, argv, OSApp::ProductName(), "UM tests" );

			let metaDataName{ "access" };
			sp<Access::IAcl> authorize = Access::LocalAcl();
			auto schema = DB::GetAppSchema( metaDataName, authorize );
			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *schema );
			QL::Configure( {schema} );
			co_await Access::Configure( schema, QL::Local(), UserPK{std::numeric_limits<UserPK::Type>::max()} );

			Access::Tests::SetSchema( schema );
			set = true;
		}
		catch( IException& e ){//don't want unhandeled exception routine.
			e.Throw();
		}
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;

	//jsonnet::Jsonnet vm;
	//vm.init();
	//string j;
	//vm.evaluateFile( "path.string()", &j );
	//Json::Parse( j, sl );

	::testing::InitGoogleTest( &argc, argv );
	try{
		bool set{};
		Startup( argc, argv, set );
		while( !set )
			std::this_thread::yield();
	}catch( const IException& e ){
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	::testing::GTEST_FLAG( filter ) = Settings::FindSV( "/testing/tests" ).value_or( "*" );
	let result = RUN_ALL_TESTS();
	Process::Shutdown( result );

	return result;
}