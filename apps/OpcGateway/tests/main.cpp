#include "gtest/gtest.h"
#include <exception>
#include <jde/fwk/settings.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/io/json.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/web/client/ClientSsl.h>
#include <jde/app/client/IAppClient.h>
#include <jde/opc/uatypes/Logger.h>
#include "../src/StartupAwait.h"
#include "../src/UAClient.h"
#include "../../AppServer/src/AppStartupAwait.h"
#include "../../OpcServer/src/StartupAwait.h"
#include "utils/helpers.h"
#include <jde/tests/SpdlogTestListener.h>
#include <jde/tests/testMain.h>
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.Opc"; }
#endif
	up<std::exception> _error;

 	Ω startup( int argc, char **argv, atomic_flag& done )ε->VoidAwait::Task{
		Logging::AddTagParser( mu<Opc::UALogParser>() );
		Process::Startup( argc, argv, "Tests.Opc", "Opc tests", true );
		Opc::Gateway::AppClient()->InitLogging( Opc::Gateway::AppClient() );
		try{
			if( Settings::FindBool("/testing/embeddedAppServer").value_or(true) ){//the fresh db enrolls the gateway+opcServer client certs every run: /access/trustedCertDirs anchors their dirs, each StartupAwait ensures its own cert, and TrustVerify rescans - no pre-anchoring here.
				co_await App::Server::AppStartupAwait{ Settings::AsObject("/http/app") };
				//the clients verify peers and the embedded server is its own root.  A deployment names its peer's cert in
				///web/client/ssl/caFile; here StartWebServer only just generated it, so register it programmatically.
				Web::Client::Ssl::AddTrustAnchor( Crypto::CryptoSettings{ "/http/app/ssl" }.Certificate.Path );
			}
			if( Settings::FindBool("/testing/embeddedOpcServer").value_or(true) ){
				//UAConfig::SetConfig snapshots trustedCertDirs at startup - create both gateway certs (UAClient transport + AppClient SslSettings auth) first, or the first run with a fresh $HOME fails BadCertificateUntrusted.
				Opc::Gateway::UAClient::EnsureCertificate( Opc::Gateway::Tests::OpcServerTarget, Settings::FindSV("/opc/urn").value_or("urn:open62541.server.application") );
				Crypto::CryptoSettings sslSettings{ Json::FindDefaultObject(Settings::AsObject("/http/gateway"), "ssl"), {} };
				Crypto::EnsureKeyCertificate( sslSettings );
				co_await Opc::Server::StartupAwait{ Settings::AsObject("/http/opcServer"), Settings::AsObject("/credentials/opcServer") };
			}

			co_await Opc::Gateway::StartupAwait{ Settings::AsObject("/http/gateway"), Settings::AsObject("/credentials/gateway") };
			Web::Client::Ssl::AddTrustAnchor( Crypto::CryptoSettings{ "/http/gateway/ssl" }.Certificate.Path );//the tests' GatewayClientSocket verifies localhost:1968 against this just-generated cert.
			done.test_and_set();
			done.notify_one();
		}
		catch( runtime_error& e ){
			_error = ToUP( move(e) );
			if( auto p = dynamic_cast<Exception*>( _error.get() ); p )
				p->Log();
			done.test_and_set();
			done.notify_one();
		}
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	atomic_flag done;
	startup( argc, argv, done );
	done.wait( false );
	int result{ EXIT_FAILURE };
	try{
		if( _error ){
			std::cerr << "startup error: " << _error->what() << std::endl;//throw *_error slices to std::exception, losing the message.
			throw *_error;
		}
		::testing::GTEST_FLAG( filter ) = Settings::FindString( "/testing/tests" ).value_or( "*" );
		Jde::SpdlogTestListener::Config( ::testing::UnitTest::GetInstance()->listeners() );
		result = CheckTestsRan( RUN_ALL_TESTS() );
	}
	catch( std::exception& e ){
		if( auto p = dynamic_cast<Exception*>( &e ); p )
			p->Log();
		Process::ExitException( move(e) );
	}
	Process::Shutdown( result );

	return result;
}