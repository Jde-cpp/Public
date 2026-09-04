//Gateway→OPC-server certificate verification (src/ServerTrust.cpp):  the verifier open62541 consults in initSecurityPolicy
//must accept a server certificate that is under a trusted directory, reject one that is not with a reason that names the
//server and the fix, and accept anything when /gateway/verifyServerCertificate is off.  No connection:  the group is
//installed on a bare UA_ClientConfig and its verifyCertificate called directly, on two certificates the harness already
//issued - the gateway's app certificate and the per-target issued certificate (tests/main.cpp EnsureCertificate).
#include <open62541/client_config_default.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/opc/uatypes/Logger.h>
#include "../../src/ServerTrust.h"
#include "../../src/UAClient.h"
#include "../../src/GatewayAppClient.h"
#include "../utils/helpers.h"
#include "Auth.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	struct ServerTrustTests : ::testing::Test{
		static constexpr Jde::Handle TestHandle{ 0x5e77 };
		Logger _logger{ TestHandle };
		UA_ClientConfig _config{};
		fs::path _trustedDir{ fs::temp_directory_path()/Ƒ("jde-servertrust-{}", Process::ProcessId()) };
		fs::path _trusted, _other;

		α SetUp()->void override{
			_config.logging = &_logger;
			_trusted = AppClient()->SslSettings->Certificate.Path;
			_other = UAClient::CryptoSettings( ServerCnnctnNK{OpcServerTarget} ).Certificate.Path;
			ASSERT_TRUE( fs::exists(_trusted) ) << _trusted;
			ASSERT_TRUE( fs::exists(_other) ) << _other;
			ASSERT_NE( Crypto::ReadCertificate(_trusted), Crypto::ReadCertificate(_other) );
			fs::create_directories( _trustedDir );
			fs::copy_file( _trusted, _trustedDir/"trusted.pem", fs::copy_options::overwrite_existing );//only one of the two is anchored.
		}
		α TearDown()->void override{
			UA_ClientConfig_clear( &_config );//runs the group's clear - frees the context.
			std::error_code ec; fs::remove_all( _trustedDir, ec );
		}
		α Verify( const fs::path& pem )->UA_StatusCode{
			let der = Crypto::ReadCertificate( pem );
			UA_ByteString bs{ der.size(), (UA_Byte*)der.data() };
			return _config.certificateVerification.verifyCertificate( &_config.certificateVerification, &bs );
		}
	};

	TEST_F( ServerTrustTests, TrustedDirAcceptsUntrustedRejects ){
		ServerTrust::Install( _config, true, {_trustedDir}, TestHandle, "opc.tcp://server.under.test:4840" );
		ASSERT_EQ( ServerTrust::AnchorCount(_config), 1u );
		EXPECT_EQ( Verify(_trusted), UA_STATUSCODE_GOOD );
		EXPECT_EQ( ServerTrust::Rejection(_config), "" );

		EXPECT_EQ( Verify(_other), UA_STATUSCODE_BADCERTIFICATEUNTRUSTED );
		let rejection = ServerTrust::Rejection( _config );
		EXPECT_NE( rejection.find("opc.tcp://server.under.test:4840"), string::npos ) << rejection;//names the server...
		EXPECT_NE( rejection.find("/gateway/verifyServerCertificate"), string::npos ) << rejection;//...and the switch.

		EXPECT_EQ( Verify(_trusted), UA_STATUSCODE_GOOD );
		EXPECT_EQ( ServerTrust::Rejection(_config), "" );//a later success clears the last rejection.
	}

	TEST_F( ServerTrustTests, NoAnchorsRejectsEverything ){
		ServerTrust::Install( _config, true, {_trustedDir/"does-not-exist"}, TestHandle, "opc.tcp://server.under.test:4840" );
		EXPECT_EQ( ServerTrust::AnchorCount(_config), 0u );
		EXPECT_EQ( Verify(_trusted), UA_STATUSCODE_BADCERTIFICATEUNTRUSTED );
		EXPECT_EQ( Verify(_other), UA_STATUSCODE_BADCERTIFICATEUNTRUSTED );
	}

	TEST_F( ServerTrustTests, OffAcceptsAnything ){
		ServerTrust::Install( _config, false, {_trustedDir}, TestHandle, "opc.tcp://server.under.test:4840" );
		EXPECT_EQ( ServerTrust::AnchorCount(_config), 0u );
		EXPECT_EQ( Verify(_trusted), UA_STATUSCODE_GOOD );
		EXPECT_EQ( Verify(_other), UA_STATUSCODE_GOOD );
		EXPECT_EQ( ServerTrust::Rejection(_config), "" );
	}


	//The live half:  a connect to the embedded OpcServer with no anchors must be refused by OUR verifier, with the detail
	//naming the server and the switch, and leave nothing behind - the same credential connects once the anchors are back.
	//IssuedToken, as TrustReloadTests:  the per-target issued cert is what every non-certificate credential presents, and
	//the OpcServer offers no Username policy.  The anchors are swapped through ServerTrust's seam rather than the setting -
	///access/trustedCertDirs is also the in-process AppServer's enrollment anchor (see the header).
	class ServerTrustLiveTests : public Auth{
	protected:
		ServerTrustLiveTests()ι:Auth{ETokenType::IssuedToken}{}
		Ω SetUpTestCase()ε->void{ _jwt = BlockAwait<Web::Client::ClientSocketAwait<Jde::Web::Jwt>,Web::Jwt>( AppClient()->Jwt() ); }
		α TearDown()ι->void override{
			ServerTrust::OverrideTrustedCertDirs( nullopt );//whatever the assertions did.
			if( _client )
				UAClient::RemoveClient( move(_client) );
		}
		α Connect( atomic_flag& flag )ι->ConnectAwait::Task{
			try{
				_exception = nullptr;
				_client = co_await UAClient::GetClient( Connection->Target, Credential{_jwt->Payload()} );
			}
			catch( Exception& e ){
				_exception = e.Move();
			}
			flag.test_and_set();
			flag.notify_all();
		}
		static optional<Web::Jwt> _jwt;
		up<Exception> _exception;
		sp<UAClient> _client;
	};
	optional<Web::Jwt> ServerTrustLiveTests::_jwt;

	TEST_F( ServerTrustLiveTests, RejectsAServerOutsideTheTrustedDirs ){
		if( auto cached = UAClient::Find(Connection->Target, Credential{_jwt->Payload()}); cached )
			UAClient::RemoveClient( move(cached) );//a client another suite left cached would skip Configuration() - the next connect must build a fresh one.
		ServerTrust::OverrideTrustedCertDirs( vector<fs::path>{ fs::temp_directory_path()/"jde-servertrust-no-anchors" } );

		atomic_flag first;
		Connect( first );
		first.wait( false );
		ASSERT_TRUE( _exception ) << "connected through an empty trust list";
		EXPECT_FALSE( _client );
		let what = string{ _exception->what() };
		EXPECT_NE( what.find("server certificate for"), string::npos ) << what;//ours, not the server rejecting the gateway's.
		EXPECT_NE( what.find(Connection->Url), string::npos ) << what;
		EXPECT_NE( what.find("/gateway/verifyServerCertificate"), string::npos ) << what;

		ServerTrust::OverrideTrustedCertDirs( nullopt );//the harness's trustedCertDirs hold the embedded server's cert.
		atomic_flag second;
		Connect( second );
		second.wait( false );
		EXPECT_FALSE( _exception ) << _exception->what();
		EXPECT_TRUE( _client );
	}
}
