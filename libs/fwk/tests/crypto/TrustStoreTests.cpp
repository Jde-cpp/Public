#include <jde/fwk/crypto/TrustStore.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <openssl/x509_vfy.h>

#define let const auto
namespace Jde::Crypto{
	constexpr ELogTags _tags = ELogTags::Test;

	struct TrustStoreTests : public ::testing::Test{
	protected:
		static α SetUpTestCase()->void;
		static α SslSettings( str publicKeyFile, str privateKeyFile, str certificateFile )ε->CryptoSettings;

		static string passcode;
		static string PublicKeyFile;
		static string PrivateKeyFile;
		static string CertificateFile;
		static string PublicKeyFile2;
		static string PrivateKeyFile2;
		static string CertificateFile2;
		static vector<byte> Der;
		static vector<byte> Der2;//distinct key pair, same subject DN as Der.
	};
	string TrustStoreTests::passcode{ "123456789" };
	string TrustStoreTests::PublicKeyFile{ _msvc ? (Process::AppDataFolder() / "trustStore-public.pem").string() : "/tmp/trustStore-public.pem" };
	string TrustStoreTests::PrivateKeyFile{ _msvc ? (Process::AppDataFolder() / "trustStore-private.pem").string() : "/tmp/trustStore-private.pem" };
	string TrustStoreTests::CertificateFile{ _msvc ? (Process::AppDataFolder() / "trustStore-cert.pem").string() : "/tmp/trustStore-cert.pem" };
	string TrustStoreTests::PublicKeyFile2{ _msvc ? (Process::AppDataFolder() / "trustStore-b-public.pem").string() : "/tmp/trustStore-b-public.pem" };
	string TrustStoreTests::PrivateKeyFile2{ _msvc ? (Process::AppDataFolder() / "trustStore-b-private.pem").string() : "/tmp/trustStore-b-private.pem" };
	string TrustStoreTests::CertificateFile2{ _msvc ? (Process::AppDataFolder() / "trustStore-b-cert.pem").string() : "/tmp/trustStore-b-cert.pem" };
	vector<byte> TrustStoreTests::Der;
	vector<byte> TrustStoreTests::Der2;

	α TrustStoreTests::SslSettings( str publicKeyFile, str privateKeyFile, str certificateFile )ε->CryptoSettings{//both pairs share the subject DN - SameSubjectAnchors depends on it.
		return CryptoSettings{ jobject{
			{"certificate", jobject{{"path", certificateFile}, {"subjectAltName", "URI:urn:my.server.application"}, {"company", "jde-cpp"}, {"country", "US"}, {"commonName", "trustStoreTests"}}},
			{"privateKey", jobject{{"path", privateKeyFile}, {"passcode", passcode}}},
			{"publicKey", jobject{{"path", publicKeyFile}}}
		}, {} };
	}

	α TrustStoreTests::SetUpTestCase()->void{
		if( !fs::exists(fs::path{PublicKeyFile}.parent_path()) )
			fs::create_directories( fs::path{PublicKeyFile}.parent_path() );
		Crypto::CreateKeyCertificate( SslSettings(PublicKeyFile, PrivateKeyFile, CertificateFile) );
		Der = Crypto::ReadCertificate( CertificateFile );
		Crypto::CreateKeyCertificate( SslSettings(PublicKeyFile2, PrivateKeyFile2, CertificateFile2) );
		Der2 = Crypto::ReadCertificate( CertificateFile2 );
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

	TEST_F( TrustStoreTests, SameSubjectAnchors ){//OpenSSL's issuer-name lookup can't distinguish same-DN anchors on its own - the exact-match path must trust both.
		TrustStore store{ false };
		store.AddCertificate( Der );
		store.AddCertificate( Der2 );
		store.Verify( Der );
		store.Verify( Der2 );
		EXPECT_TRUE( store.IsTrusted(Der) );
		EXPECT_TRUE( store.IsTrusted(Der2) );
	}

	TEST_F( TrustStoreTests, SerialsRandomized ){//IssueCertificate must never reuse a constant serial - same-DN+same-serial re-issues are indistinguishable by subject in an X509_STORE.
		using X509Ptr = up<X509, decltype(&::X509_free)>;
		auto parse = []( const vector<byte>& der )->X509Ptr{
			auto p = (const unsigned char*)der.data();
			return X509Ptr{ ::d2i_X509(nullptr, &p, (long)der.size()), ::X509_free };
		};
		let cert = parse( Der ), cert2 = parse( Der2 );
		ASSERT_TRUE( cert && cert2 );
		let sn = ::X509_get_serialNumber( cert.get() ), sn2 = ::X509_get_serialNumber( cert2.get() );
		EXPECT_NE( ::ASN1_INTEGER_cmp(sn, sn2), 0 );
		up<ASN1_INTEGER, decltype(&::ASN1_INTEGER_free)> one{ ::ASN1_INTEGER_new(), ::ASN1_INTEGER_free };
		::ASN1_INTEGER_set( one.get(), 1 );
		EXPECT_NE( ::ASN1_INTEGER_cmp(sn, one.get()), 0 );//the historical hardcoded value.
		EXPECT_NE( ::ASN1_INTEGER_cmp(sn2, one.get()), 0 );
	}

	TEST_F( TrustStoreTests, OsStoreLoads ){
		TrustStore store;
		if( _msvc )
			EXPECT_GT( store.CertCount(), (uint)0 );//winstore load is eager and ROOT is always populated.
		else if( store.CertCount()==0 )
			WARN( "OS trust store is empty - set SSL_CERT_FILE/SSL_CERT_DIR if OpenSSL's default paths are wrong." );
	}
}
