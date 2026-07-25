#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include <jde/fwk.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/crypto/CryptoSettings.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/app/client/IAppClient.h>
#include <jde/app/client/appClient.h>
#include <jde/opc/uatypes/Logger.h>
#include "SoakAppClient.h"
#include "SoakRunner.h"

#define let const auto
#ifndef _MSC_VER
	α Jde::Process::ProductName()ι->sv{ return "Opc.Soak"; }
#endif

namespace Jde::Opc::Gateway::Soak{
	constexpr ELogTags _tags{ ELogTags::Test };
	//Creates the gateway's client certificate for the soak target. Must run before OpcServer starts: OpcServer
	//snapshots trustedCertDirs at startup, but the gateway only creates the cert lazily at first connect - too late
	//in a split-process run. Mirrors UAClient::EnsureCertificate with the gateway's RootSslDir spelled out.
	Ω createGatewayCert()ε->void{
		let target = Settings::FindString( "/soak/target" ).value_or( "OpcSoak" );
		let urn = Settings::FindString( "/opc/urn" ).value_or( "urn:open62541.server.application" );
		let product = Settings::FindString( "/soak/gatewayProduct" ).value_or( "OpcGateway" );
		const fs::path root = Process::ProgramDataFolder()/Process::CompanyRootDir()/product/"ssl";
		for( sv sub : {"certs", "private", "public"} )
			fs::create_directories( root/sub );
		let passcode = Process::GetEnv( "JDE_PASSCODE" ).value_or( "" );
		let privateKeyFile = root/Ƒ("private/{}.pem", target);
		if( !fs::exists(privateKeyFile) )
			Crypto::CreateKey( root/Ƒ("public/{}.pem", target), privateKeyFile, passcode );
		let certificateFile = root/Ƒ("certs/{}.pem", target);
		if( !fs::exists(certificateFile) )
			Crypto::CreateCertificate( certificateFile, privateKeyFile, passcode, Ƒ("URI:{}", urn), "jde-cpp", "US", "localhost" );
		INFO( "Gateway certificate ready: {}.", certificateFile.string() );
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	using namespace Jde::Opc::Gateway;
	Logging::AddTagParser( mu<Opc::UALogParser>() );
	int exitCode{ EXIT_FAILURE };
	try{
		Process::Startup( argc, argv, "Jde.Opc.Soak", "OpcGateway soak driver" );
		if( Process::FindArg("-createCert") ){
			Soak::createGatewayCert();
			exitCode = EXIT_SUCCESS;
		}
		else{
			auto client = Soak::AppClient();
			client->InitLogging( client );
			Crypto::CryptoSettings ssl{ Json::FindDefaultObject(Settings::AsObject("/http"), "ssl") };
			if( !fs::exists(ssl.PrivateKeyPath) ){
				ssl.CreateDirectories();
				Crypto::CreateKeyCertificate( ssl );
			}
			client->SslSettings = ssl;
			client->SetUserName( jobject{Settings::AsObject("/credentials")} );
			Execution::Run();//without proto/remote logging or a web server, nothing else starts the executor thread the socket/http awaitables need.
			BlockVoidAwait( App::Client::ConnectAwait{client, false} );
			if( Process::FindArg("-grant") ){//soak.sh runs this after OpcServer's first boot (which registers the nodeIds resource), then restarts OpcServer to load the acl.
				Soak::GrantWriteRights( client );
				exitCode = EXIT_SUCCESS;
			}
			else
				exitCode = Soak::Run( client );
		}
	}
	catch( exception& e ){
		exitCode = Process::ExitException( move(e) );
	}
	Process::Shutdown( exitCode );
	return exitCode;
}
