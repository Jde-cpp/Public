#include "globals.h"
#include <openssl/x509_vfy.h>
#include <jde/access/server/accessServer.h>
#include <jde/access/server/awaits/LoginAwait.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/fwk/crypto/TrustStore.h>
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>

#define let const auto
namespace Jde::Access::Tests{
	struct LoginTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		struct KeyCert{
			Crypto::PublicKey Key;
			vector<byte> Der;
		};
		Ω Generate( sv name, sv commonName, sv altName = "URI:urn:my.server.application" )ε->KeyCert;
		static KeyCert Untrusted;//never anchored - enrollment must reject it.
		static KeyCert Trusted;//AddCertificate'd into Server::Trust().
	};
	LoginTests::KeyCert LoginTests::Untrusted;
	LoginTests::KeyCert LoginTests::Trusted;

	Ω testsDir()ι->fs::path{ return _msvc ? Process::AppDataFolder() : fs::path{"/tmp"}; }

	α LoginTests::Generate( sv name, sv commonName, sv altName )ε->KeyCert{
		let dir = testsDir();
		if( !fs::exists(dir) )
			fs::create_directories( dir );
		let certificateFile = dir / Ƒ( "loginTests-{}-cert.pem", name );
		const jobject ssl{
			{ "certificate", jobject{{"path", certificateFile.string()}, {"subjectAltName", altName}, {"company", "jde-cpp"}, {"country", "US"}, {"commonName", commonName}} },
			{ "privateKey", jobject{{"path", (dir/Ƒ("loginTests-{}-private.pem", name)).string()}, {"passcode", "123456789"}} },
			{ "publicKey", jobject{{"path", (dir/Ƒ("loginTests-{}-public.pem", name)).string()}} }
		};
		Crypto::CreateKeyCertificate( Crypto::CryptoSettings{ssl, {}} );
		auto der = Crypto::ReadCertificate( certificateFile );
		auto key = Crypto::ExtractPublicKey( der, SRCE_CUR );
		return { move(key), move(der) };
	}
	α LoginTests::SetUpTestCase()ε->void{//fresh keys each run - never pre-enrolled. distinct CNs - the CN is the unique identity target.
		Untrusted = Generate( "a", "loginTests-untrusted" );
		Trusted = Generate( "b", "loginTests-trusted", "email:trusted@jde-cpp.com,otherName:1.3.6.1.4.1.311.20.2.3;UTF8:trusted-upn@jde-cpp.com,URI:urn:my.server.application" );
	}

	α keyLogin( Crypto::PublicKey key, vector<byte> certificate )ε->UserPK{
		return BlockTAwait<UserPK>( Server::LoginAwait{move(key), move(certificate), "login tests user"} );
	}

	TEST_F( LoginTests, NoCert_Rejected ){
		EXPECT_ANY_THROW( keyLogin(Untrusted.Key, {}) );
	}

	TEST_F( LoginTests, UntrustedCert_Rejected ){
		try{
			keyLogin( Untrusted.Key, vector<byte>{Untrusted.Der} );
			FAIL() << "keyLogin should have thrown.";
		}catch( const Exception& e ){
			EXPECT_EQ( e.Code(), (uint32)X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT );
		}
	}

	TEST_F( LoginTests, MismatchedCert_Rejected ){//a trusted cert must bind the jwt key.
		Server::Trust().AddCertificate( Trusted.Der );
		EXPECT_ANY_THROW( keyLogin(Untrusted.Key, vector<byte>{Trusted.Der}) );
	}

	TEST_F( LoginTests, TrustedCert_Enrolls ){
		Server::Trust().AddCertificate( Trusted.Der );
		let userPK = keyLogin( Trusted.Key, vector<byte>{Trusted.Der} );
		ASSERT_TRUE( userPK.Value );
		let user = Select( "users", userPK.Value, GetRoot(), "email issuer subjectAlt distinguished expiration" );//the cert is the identity authority: UPN→name, CN→target.
		EXPECT_EQ( Json::AsSV(user, "name"), "trusted-upn@jde-cpp.com" );
		EXPECT_EQ( Json::AsSV(user, "target"), "loginTests-trusted" );
		EXPECT_EQ( Json::AsSV(user, "email"), "trusted@jde-cpp.com" );
		EXPECT_EQ( Json::AsSV(user, "distinguished"), "CN=loginTests-trusted,O=jde-cpp,C=US" );//subject DN.
		EXPECT_EQ( Json::AsSV(user, "issuer"), Json::AsSV(user, "distinguished") );//self-signed.
		//the SAN, openssl config syntax - the columns hold two different things and must not be interchangeable.
		EXPECT_EQ( Json::AsSV(user, "subjectAlt"), "email:trusted@jde-cpp.com,otherName:msUPN;UTF8:trusted-upn@jde-cpp.com,URI:urn:my.server.application" );
		EXPECT_FALSE( user.at("expiration").is_null() );//notAfter - CreateCertificate issues 365-day certs.
		let again = keyLogin( Trusted.Key, {} );//existing user bypasses the gate.
		ASSERT_EQ( userPK.Value, again.Value );
		PurgeUser( userPK, GetRoot() );
	}

	TEST_F( LoginTests, EmailFallback_NoUpn ){//no UPN in the SAN - name falls back to the email.
		let pair = Generate( "c", "loginTests-email", "email:fallback@jde-cpp.com" );
		Server::Trust().AddCertificate( pair.Der );
		let userPK = keyLogin( pair.Key, vector<byte>{pair.Der} );
		let user = Select( "users", userPK.Value, GetRoot(), "email" );
		EXPECT_EQ( Json::AsSV(user, "name"), "fallback@jde-cpp.com" );
		EXPECT_EQ( Json::AsSV(user, "email"), "fallback@jde-cpp.com" );
		PurgeUser( userPK, GetRoot() );
	}

	TEST_F( LoginTests, CnFallback_NoSan ){//no UPN or email - name falls back to the CN.
		let pair = Generate( "d", "loginTests-cn" );
		Server::Trust().AddCertificate( pair.Der );
		let userPK = keyLogin( pair.Key, vector<byte>{pair.Der} );
		let user = Select( "users", userPK.Value, GetRoot(), "email" );
		EXPECT_EQ( Json::AsSV(user, "name"), "loginTests-cn" );
		EXPECT_EQ( Json::AsSV(user, "target"), "loginTests-cn" );
		EXPECT_TRUE( Json::FindSV(user, "email").value_or("").empty() );
		PurgeUser( userPK, GetRoot() );
	}

	TEST_F( LoginTests, EmptyCn_Rejected ){//the CN is the identity target - a trusted cert without one cannot enroll.
		let pair = Generate( "e", "" );
		Server::Trust().AddCertificate( pair.Der );
		EXPECT_ANY_THROW( keyLogin(pair.Key, vector<byte>{pair.Der}) );
	}

	TEST_F( LoginTests, LocalhostCn_Rejected ){//a generic CN means the operator never chose an identity - it cannot be unique across clients.
		let pair = Generate( "g", "localhost" );
		Server::Trust().AddCertificate( pair.Der );
		EXPECT_ANY_THROW( keyLogin(pair.Key, vector<byte>{pair.Der}) );
	}

	TEST_F( LoginTests, DirAnchor_Enrolls ){//the production anchoring path: the operator copies a client cert into /access/trustedCertDirs - TrustVerify rescans on failure, so no AddCertificate and no restart.
		let pair = Generate( "f", "loginTests-dir" );
		let dir = testsDir()/"loginTests-trusted";
		fs::create_directories( dir );
		fs::copy_file( testsDir()/"loginTests-f-cert.pem", dir/"loginTests-f-cert.pem", fs::copy_options::overwrite_existing );
		Settings::Set( "/access/trustedCertDirs", jarray{dir.string()} );
		let userPK = keyLogin( pair.Key, vector<byte>{pair.Der} );
		let user = Select( "users", userPK.Value, GetRoot(), "email" );
		EXPECT_EQ( Json::AsSV(user, "target"), "loginTests-dir" );
		PurgeUser( userPK, GetRoot() );
	}
}
