#include <jde/crypto/OpenSsl.h>
#include "../../src/crypto/OpenSslInternal.h"
//#include "../../src/crypto/OpenSslInternal.h"
#include "../../../Framework/source/Settings.h"

#define var const auto
namespace Jde::Crypto{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "test" ) };
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
	string OpenSslTests::PublicKeyFile{ _msvc ? (OSApp::ApplicationDataFolder() / "public.pem").string() : "/tmp/public.pem" };
	string OpenSslTests::PrivateKeyFile{ _msvc ? (OSApp::ApplicationDataFolder() / "private.pem").string() : "/tmp/private.pem" };
	string OpenSslTests::CertificateFile{ _msvc ? (OSApp::ApplicationDataFolder() / "cert.pem").string() : "/tmp/cert.pem" };


	α OpenSslTests::SetUpTestCase()->void{
		var clear = Settings::Get<bool>( "cryptoTests/clear" ).value_or( true );
		INFO( "clear={}", clear );
		INFO( "clear={}", HeaderPayload );
		LOG( ELogLevel::Information, _logTag, "clear={}", clear );
		Logging::Log(Logging::MessageBase("clear={}", ELogLevel::Information, __FILE__, __func__, __LINE__), _logTag, true);
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
		// array<unsigned char, SHA256_DIGEST_LENGTH> md;
		// auto pDigest = SHA256( (unsigned char*)HeaderPayload.data(), HeaderPayload.size(), md.data() ); CHECK_NULL( pDigest );
		// KeyPtr pKey{ Internal::ReadPrivateKey(PrivateKeyFile) };
		// CtxPtr ctx{ NewCtx(pKey) };
		// CALL( EVP_PKEY_sign_init(ctx.get()) );
		// CALL( EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) );
		// CALL( EVP_PKEY_CTX_set_signature_md(ctx.get(), EVP_sha256()) );
		// uint siglen;
		// CALL( EVP_PKEY_sign(ctx.get(), nullptr, &siglen, pDigest, md.size()) );
		// Signature signature(siglen);
		// CALL( EVP_PKEY_sign(ctx.get(), (unsigned char*)signature.data(), &siglen, pDigest, md.size()) );
		// auto x = Str::Encode64( signature );
		var signature = Crypto::RsaSign( HeaderPayload, PrivateKeyFile );
		auto [modulus2, exponent2] = Crypto::ModulusExponent( PublicKeyFile );
		//vector<unsigned char> modulus = modulus2;//natvis issues.
		//vector<unsigned char> exponent = exponent2;

		Verify( modulus2, exponent2, HeaderPayload, signature );
	}

	TEST_F( OpenSslTests, Certificate ){
		ReadCertificate( CertificateFile );
		//ReadCertificate( "/tmp/cert2.pem" );
	}
	TEST_F( OpenSslTests, PrivateKey ){
		Crypto::ReadPrivateKey( PrivateKeyFile );
	}


//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
}