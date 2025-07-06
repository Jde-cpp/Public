#include "gtest/gtest.h"
#include <jde/framework/settings.h>
#include "../../../../Framework/source/Cache.h"
#include "../../../../AppServer/source/AppStartupAwait.h"
#include <jde/framework/thread/execution.h>
#include <jde/db/db.h>
//#include <jde/access/access.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/opc/uatypes/Logger.h>
#include <jde/opc/opc.h>
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
 	α Startup( int argc, char **argv, atomic_flag& done )ε->VoidAwait<>::Task{
#ifdef _MSC_VER
		ASSERT( Settings::FindNumber<uint>("/workers/drive/threads").value_or(0)>0 )
#endif
		Logging::AddTagParser( mu<Opc::UALogParser>() );
		OSApp::Startup( argc, argv, "Tests.Opc", "Opc tests", true );
/*		Crypto::CryptoSettings settings{ "http/ssl" };
		if( !fs::exists(settings.PrivateKeyPath) ){
			settings.CreateDirectories();
			Crypto::CreateKey( settings.PublicKeyPath, settings.PrivateKeyPath, settings.Passcode );
		}
*/
//		let metaDataName{ "opc" };
		keepExecuterAlive();
		try{
			// auto schema = DB::GetAppSchema( metaDataName, App::Client::RemoteAcl() );
			// Opc::Configure( schema );
			// auto accessSchema = DB::GetAppSchema( "access", App::Client::RemoteAcl() );
			co_await App::AppStartupAwait{ Settings::AsObject("/http/app") };
			co_await Opc::Server::StartupAwait{ Settings::AsObject("/http/opcServer") };
			co_await Opc::Gateway::StartupAwait{ Settings::AsObject("/http/gateway") };
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
	Startup( argc, argv, done );
	done.wait( false );
	int result = 0;
	try{
		if( _exception )
			throw *_exception;
		::testing::GTEST_FLAG( filter ) = Settings::FindString( "/testing/tests" ).value_or( "*" );
		result = RUN_ALL_TESTS();
	}
	catch( exception& e ){
		if( auto p = dynamic_cast<IException*>(&e); p ){
			p->Log();
			result = p->Code;
		}
		else
			std::cerr << e.what() << std::endl;
	}
	Process::Shutdown( result );

	return result;
}