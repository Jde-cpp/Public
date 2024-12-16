#include "gtest/gtest.h"
#include <jde/framework/settings.h>
#include "../../../../Framework/source/Cache.h"
#include <jde/access/access.h>
#include <jde/app/client/AppClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/db/db.h>
#include <jde/opc/uatypes/Logger.h>
#include <jde/framework/thread/execution.h>
#include "helpers.h"
#define let const auto

namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.Opc"; }

 	α Startup( int argc, char **argv )ι->Access::ConfigureAwait::Task{
#ifdef _MSC_VER
		ASSERT( Settings::Get<uint>("workers/drive/threads")>0 )
#endif
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		TagParser( Opc::LogTagParser );
		OSApp::Startup( argc, argv, "Tests.Opc", "Opc tests" );
		Crypto::CryptoSettings settings{ "http/ssl" };
		if( !fs::exists(settings.PrivateKeyPath) ){
			settings.CreateDirectories();
			Crypto::CreateKey( settings.PublicKeyPath, settings.PrivateKeyPath, settings.Passcode );
		}
		using namespace App::Client;
		Connect();
		Execution::Run();

		while( !AppClientSocketSession::Instance() || !AppClientSocketSession::Instance()->Id() )
			std::this_thread::yield();

		let metaDataName{ "opc" };
		sp<Access::IAcl> authorize = Access::RemoteAcl();
		auto schema = DB::GetAppSchema( metaDataName, authorize );
		if( Settings::FindBool("/testing/recreateDB").value_or(false) )
			DB::NonProd::Recreate( *schema );
		co_await Access::Configure( schema );
		QL::Configure( {schema} );
		Opc::AddHook();
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	int result;
	{
		let p=Settings::Get<string>( "testing/tests" );
		let filter = p ? *p : "*";
		::testing::GTEST_FLAG( filter ) = filter;
	  result = RUN_ALL_TESTS();
		Process::Shutdown( result );
	}
	return result;
}