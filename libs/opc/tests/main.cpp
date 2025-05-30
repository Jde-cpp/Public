#include "gtest/gtest.h"
#include <jde/framework/settings.h>
#include "../../../../Framework/source/Cache.h"
#include <jde/framework/thread/execution.h>
#include <jde/db/db.h>
#include <jde/access/access.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/opc/uatypes/Logger.h>
#include <jde/opc/opc.h>
#include "helpers.h"
#define let const auto

namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.Opc"; }

 	α Startup( int argc, char **argv )ε->void{
#ifdef _MSC_VER
		ASSERT( Settings::FindNumber<uint>("/workers/drive/threads").value_or(0)>0 )
#endif
		TagFromString( Opc::TagFromString );
		TagToString( Opc::TagToString );
		OSApp::Startup( argc, argv, "Tests.Opc", "Opc tests", true );
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
		try{
			auto schema = DB::GetAppSchema( metaDataName, App::Client::RemoteAcl() );
			Opc::Configure( schema );
			auto accessSchema = DB::GetAppSchema( "access", App::Client::RemoteAcl() );

			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *schema, QL::Local() );
			else if( Settings::FindBool("/dbServers/sync").value_or(false) )
				DB::SyncSchema( *schema, QL::Local() );
			auto await = Access::Configure( accessSchema, {schema}, App::Client::QLServer(), {UserPK::System} );
			BlockVoidAwait<Access::ConfigureAwait>( move(await) );
			QL::Configure( {schema} );
			Opc::AddHook();
		}
		catch( IException& e ){
			e.Log();
			e.Throw();
		}
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	::testing::GTEST_FLAG( filter ) = Settings::FindString( "/testing/tests" ).value_or( "*" );
	int result = RUN_ALL_TESTS();
	Process::Shutdown( result );

	return result;
}