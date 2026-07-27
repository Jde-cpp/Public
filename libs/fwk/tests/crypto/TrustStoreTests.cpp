#include <jde/fwk/crypto/TrustStore.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <openssl/x509_vfy.h>

#define let const auto
namespace Jde::Crypto{
	constexpr ELogTags _tags = ELogTags::Test;

	struct TrustStoreTests : public ::testing::Test{
	protected:
		static α SetUpTestCase()->void;

		static string passcode;
		static string PublicKeyFile;
		static string PrivateKeyFile;
		static string CertificateFile;
		static vector<byte> Der;
	};
	string TrustStoreTests::passcode{ "123456789" };
	string TrustStoreTests::PublicKeyFile{ _msvc ? (Process::AppDataFolder() / "trustStore-public.pem").string() : "/tmp/trustStore-public.pem" };
	string TrustStoreTests::PrivateKeyFile{ _msvc ? (Process::AppDataFolder() / "trustStore-private.pem").string() : "/tmp/trustStore-private.pem" };
	string TrustStoreTests::CertificateFile{ _msvc ? (Process::AppDataFolder() / "trustStore-cert.pem").string() : "/tmp/trustStore-cert.pem" };
	vector<byte> TrustStoreTests::Der;

	α TrustStoreTests::SetUpTestCase()->void{
		if( !fs::exists(fs::path{PublicKeyFile}.parent_path()) )
			fs::create_directories( fs::path{PublicKeyFile}.parent_path() );
		Crypto::CreateKey( PublicKeyFile, PrivateKeyFile, passcode );
		Crypto::CreateCertificate( CertificateFile, PrivateKeyFile, passcode, "URI:urn:my.server.application", "jde-cpp", "US", "localhost" );
		Der = Crypto::ReadCertificate( CertificateFile );
	}

	TEST_F( TrustStoreTests, SelfSignedNotTrusted ){
		TrustStore store;
		EXPECT_FALSE( store.IsTrusted(Der) );
		try{
			store.Verify( Der );
			FAIL() << "Verify should have thrown.";
		}catch( const Exception& e ){
			EXPECT_EQ( e.Code(), (uint32)X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT );
		}
	}

	TEST_F( TrustStoreTests, AddCertificateTrusts ){
		TrustStore store{ false };
		EXPECT_FALSE( store.IsTrusted(Der) );
		store.AddCertificate( Der );
		store.Verify( Der );
		EXPECT_TRUE( store.IsTrusted(Der) );
	}

	TEST_F( TrustStoreTests, OsStoreLoads ){
		TrustStore store;
		if( _msvc )
			EXPECT_GT( store.CertCount(), (uint)0 );//winstore load is eager and ROOT is always populated.
		else if( store.CertCount()==0 )
			WARN( "OS trust store is empty - set SSL_CERT_FILE/SSL_CERT_DIR if OpenSSL's default paths are wrong." );
	}
}
