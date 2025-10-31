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

		static string HeaderPayload;
		static string passcode;
		static string PublicKeyFile;
		static string PrivateKeyFile;
		static string CertificateFile;
	};
	string OpenSslTests::HeaderPayload{ "secret stuff" };
	string OpenSslTests::passcode{ "123456789" };
	string OpenSslTests::PublicKeyFile{ _msvc ? (Process::ApplicationDataFolder() / "public.pem").string() : "/tmp/public.pem" };
	string OpenSslTests::PrivateKeyFile{ _msvc ? (Process::ApplicationDataFolder() / "private.pem").string() : "/tmp/private.pem" };
	string OpenSslTests::CertificateFile{ _msvc ? (Process::ApplicationDataFolder() / "cert.pem").string() : "/tmp/cert.pem" };


	α OpenSslTests::SetUpTestCase()->void{
		let clear = Settings::FindBool( "cryptoTests/clear" ).value_or( true );
		INFO( "clear={}", clear );
		INFO( "HeaderPayload={}", HeaderPayload );
		if( clear || (!fs::exists(PublicKeyFile) || !fs::exists(PrivateKeyFile)) ){
			if( !fs::exists(fs::path{PublicKeyFile}.parent_path()) )
				fs::create_directories( fs::path{PublicKeyFile}.parent_path() );
			Crypto::CreateKey( PublicKeyFile, PrivateKeyFile, passcode );
			INFO( "Created keys {} {}", PublicKeyFile, PrivateKeyFile );
		}
		if( clear || !fs::exists(CertificateFile) ){
			Crypto::CreateCertificate( CertificateFile, PrivateKeyFile, passcode, "URI:urn:my.server.application", "jde-cpp", "US", "localhost" );
			INFO( "Created certificate {}", CertificateFile );
		}
	}

	TEST_F( OpenSslTests, Main ){
		let signature = Crypto::RsaSign( HeaderPayload, PrivateKeyFile );
		auto publicKey = Crypto::ReadPublicKey( PublicKeyFile );

		Crypto::Verify( publicKey, HeaderPayload, signature );
	}

	TEST_F( OpenSslTests, Certificate ){
		auto bytes = ReadCertificate( CertificateFile );
		ExtractPublicKey( bytes );
	}
	TEST_F( OpenSslTests, PrivateKey ){
		Crypto::ReadPrivateKey( PrivateKeyFile, {} );
	}
}