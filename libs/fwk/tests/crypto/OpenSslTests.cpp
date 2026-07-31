#include <jde/fwk/crypto/OpenSsl.h>
#include "../../src/crypto/OpenSslInternal.h"
#include <jde/fwk/settings.h>

#define let const auto
namespace Jde::Crypto{
	constexpr ELogTags _tags = ELogTags::Test;
	using namespace Crypto::Internal;

	struct OpenSslTests : public ::testing::Test{
	protected:
		OpenSslTests() {}
		~OpenSslTests() override{}

		static α SetUpTestCase()->void;
		α SetUp()->void override{};
		α TearDown()->void override{}

		static α GetModulusExponent( fs::path publicKey )ε->tuple<vector<unsigned char>,vector<unsigned char>>;
		static α SslSettings( str publicKeyFile, str privateKeyFile, str certificateFile, sv commonName, sv subjectAltName="URI:urn:my.server.application" )ε->CryptoSettings;

		static string HeaderPayload;
		static string passcode;
		static string PublicKeyFile;
		static string PrivateKeyFile;
		static string CertificateFile;
	};
	string OpenSslTests::HeaderPayload{ "secret stuff" };
	string OpenSslTests::passcode{ "123456789" };
	string OpenSslTests::PublicKeyFile{ _msvc ? (Process::AppDataFolder() / "public.pem").string() : "/tmp/public.pem" };
	string OpenSslTests::PrivateKeyFile{ _msvc ? (Process::AppDataFolder() / "private.pem").string() : "/tmp/private.pem" };
	string OpenSslTests::CertificateFile{ _msvc ? (Process::AppDataFolder() / "cert.pem").string() : "/tmp/cert.pem" };


	α OpenSslTests::SslSettings( str publicKeyFile, str privateKeyFile, str certificateFile, sv commonName, sv subjectAltName )ε->CryptoSettings{
		return CryptoSettings{ jobject{
			{"certificate", jobject{{"path", certificateFile}, {"subjectAltName", subjectAltName}, {"company", "jde-cpp"}, {"country", "US"}, {"commonName", commonName}}},
			{"privateKey", jobject{{"path", privateKeyFile}, {"passcode", passcode}}},
			{"publicKey", jobject{{"path", publicKeyFile}}}
		}, {} };
	}

	α OpenSslTests::SetUpTestCase()->void{
		let clear = Settings::FindBool( "cryptoTests/clear" ).value_or( true );
		INFO( "clear={}", clear );
		INFO( "HeaderPayload={}", HeaderPayload );
		let settings = SslSettings( PublicKeyFile, PrivateKeyFile, CertificateFile, "openSslTests" );//the CN is the identity target - never "localhost".
		if( clear || (!fs::exists(PublicKeyFile) || !fs::exists(PrivateKeyFile)) ){
			if( !fs::exists(fs::path{PublicKeyFile}.parent_path()) )
				fs::create_directories( fs::path{PublicKeyFile}.parent_path() );
			Crypto::CreateKey( settings, SRCE_CUR );
			INFO( "Created keys {} {}", PublicKeyFile, PrivateKeyFile );
		}
		if( clear || !fs::exists(CertificateFile) ){
			Crypto::IssueCertificate( settings );
			INFO( "Created certificate {}", CertificateFile );
		}
	}

	TEST_F( OpenSslTests, Main ){
		let signature = Crypto::RsaSign( HeaderPayload, PrivateKeyFile, passcode );
		auto publicKey = Crypto::ReadPublicKey( PublicKeyFile );

		Crypto::Verify( publicKey, HeaderPayload, signature );
	}

	TEST_F( OpenSslTests, Certificate ){
		auto bytes = ReadCertificate( CertificateFile );
		ExtractPublicKey( bytes, SRCE_CUR );
	}
	TEST_F( OpenSslTests, ExtractInfo ){
		let publicKeyFile = _msvc ? (Process::AppDataFolder()/"extractInfo-public.pem").string() : "/tmp/extractInfo-public.pem";
		let privateKeyFile = _msvc ? (Process::AppDataFolder()/"extractInfo-private.pem").string() : "/tmp/extractInfo-private.pem";
		let certificateFile = _msvc ? (Process::AppDataFolder()/"extractInfo-cert.pem").string() : "/tmp/extractInfo-cert.pem";
		Crypto::CreateKeyCertificate( SslSettings(publicKeyFile, privateKeyFile, certificateFile, "extract-info-cn", "email:tester@jde-cpp.com,otherName:1.3.6.1.4.1.311.20.2.3;UTF8:upn-tester@jde-cpp.com,URI:urn:my.server.application") );
		let info = Crypto::Certificate{ ReadCertificate(certificateFile) };
		EXPECT_EQ( info.CommonName, "extract-info-cn" );
		EXPECT_EQ( info.Email, "tester@jde-cpp.com" );
		EXPECT_EQ( info.Upn, "upn-tester@jde-cpp.com" );
		EXPECT_EQ( info.DistinguishedName, "CN=extract-info-cn,O=jde-cpp,C=US" );
		EXPECT_EQ( info.Issuer, info.DistinguishedName );//self-signed.
		//the SAN must come back in the openssl config syntax it was issued with, so a parsed cert can be re-issued.
		EXPECT_EQ( info.SubjectAltName, "email:tester@jde-cpp.com,otherName:msUPN;UTF8:upn-tester@jde-cpp.com,URI:urn:my.server.application" );
		EXPECT_EQ( info.SanUri(), "urn:my.server.application" );//not the whole SAN - that was the applicationUri bug.
		EXPECT_GT( info.Expiration, Clock::now() );//CreateCertificate issues 365-day certs.
		let plain = Crypto::Certificate{ ReadCertificate(CertificateFile) };//fixture cert has a URI-only SAN.
		EXPECT_TRUE( plain.Email.empty() );
		EXPECT_TRUE( plain.Upn.empty() );
		EXPECT_EQ( plain.SubjectAltName, "URI:"+plain.SanUri() );//single entry - SanUri is the whole thing bar the prefix.
	}
	TEST_F( OpenSslTests, PrivateKey ){
		Crypto::ReadPrivateKey( PrivateKeySettings{PrivateKeyFile, passcode} );
		//the key was created with a passcode - it must be encrypted at rest, i.e. unreadable without it.
		EXPECT_THROW( Crypto::ReadPrivateKey(PrivateKeySettings{PrivateKeyFile, string{}}), Exception );
	}
	TEST_F( OpenSslTests, Random ){
		array<unsigned char,16> a{}, b{};
		Crypto::Random( a.data(), a.size() );
		Crypto::Random( b.data(), b.size() );
		EXPECT_NE( a, b );//2⁻¹²⁸ false-failure odds.
		EXPECT_NE( a, (array<unsigned char,16>{}) );
		Crypto::Random<uint32_t>();
	}
}