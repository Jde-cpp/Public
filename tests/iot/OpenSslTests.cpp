#include <jde/crypto/OpenSsl.h>
#include "../../src/crypto/OpenSslInternal.h"
#include "../../../Ssl/source/Ssl.h"

#define var const auto
namespace Jde::Iot{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };
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
			Crypto::CreateKey( PublicKeyFile, PrivateKeyFile, passcode );
			INFO( "Created keys {} {}", PublicKeyFile, PrivateKeyFile );
		}
		if( clear || !fs::exists(CertificateFile) ){
			Crypto::CreateCertificate( CertificateFile, PrivateKeyFile, passcode, "jde-cpp", "altName", "US", "localhost" );
			INFO( "Created certificate {}", CertificateFile );
		}
	}
	//using namespace Jde::Crypto;
	TEST_F( OpenSslTests, Main ){
		auto [modulus2, exponent2] = GetModulusExponent( PublicKeyFile );
		vector<unsigned char> modulus = modulus2;
		vector<unsigned char> exponent = exponent2;
		constexpr uint mdlen = SHA256_DIGEST_LENGTH;
		unsigned char md[mdlen];
		auto pDigest = SHA256( (unsigned char*)HeaderPayload.data(), HeaderPayload.size(), md ); CHECK_NULL( pDigest );
		KeyPtr pKey{ Crypto::Internal::ReadPrivateKey(PrivateKeyFile) };
		CtxPtr ctx{ NewCtx(pKey) };
		CALL( EVP_PKEY_sign_init(ctx.get()) );
		CALL( EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) );	
		CALL( EVP_PKEY_CTX_set_signature_md( ctx.get(), EVP_sha256()) );
		uint siglen;
		CALL( EVP_PKEY_sign(ctx.get(), nullptr, &siglen, pDigest, mdlen) );
		string signature; signature.resize( siglen );
		CALL( EVP_PKEY_sign(ctx.get(), (unsigned char*)signature.data(), &siglen, pDigest, mdlen) );
		auto x = Ssl::Encode64( signature );
		Crypto::Verify( modulus, exponent, HeaderPayload, signature );
	}

	TEST_F( OpenSslTests, Certificate ){
		Crypto::ReadCertificate( CertificateFile );
		//ReadCertificate( "/tmp/cert2.pem" );
	}
	TEST_F( OpenSslTests, PrivateKey ){
		Crypto::ReadPrivateKey( PrivateKeyFile );
	}


#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	α OpenSslTests::GetModulusExponent( fs::path publicKey )ε->tuple<vector<unsigned char>,vector<unsigned char>>{
		KeyPtr pKey{ Crypto::Internal::ReadPublicKey(publicKey) };
		BIGNUM* n{}, *e{};
		CALL( EVP_PKEY_get_bn_param(pKey.get(), "e", &e) );
		CALL( EVP_PKEY_get_bn_param(pKey.get(), "n", &n) );
		BNPtr pN( n, ::BN_free );
		BNPtr pE( e, ::BN_free );
		vector<unsigned char> modulus( BN_num_bytes(n) );
		BN_bn2bin( pN.get(), modulus.data() );
		vector<unsigned char> exponent( BN_num_bytes(e) );
		BN_bn2bin( pE.get(), exponent.data() );
		return { modulus, exponent };
	}
}