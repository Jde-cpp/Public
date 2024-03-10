#include <jde/crypto/OpenSsl.h>
#include "../../../Ssl/source/Ssl.h"

#define var const auto
#define CALL( call ) if( int rc=call; rc!=1 ) throw Crypto::OpenSslException( "#call - {}", rc, SRCE_CUR, Crypto::OpenSslException::CurrentError() )
#define CHECK_NULL( p ) THROW_IFX( !p, Crypto::OpenSslException("{}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) )
namespace Jde::Crypto{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };
	using namespace Crypto::Internal;

	struct OpenSslTests : public ::testing::Test{
	protected:
		OpenSslTests() {}
		~OpenSslTests() override{}

		void SetUp() override {}
		void TearDown() override {}
	};

	α GetModulusExponent( fs::path publicKey )ε->tuple<vector<unsigned char>,vector<unsigned char>>;
	string headerPayload = "secret stuff";
	TEST_F( OpenSslTests, Main ){
		const fs::path publicKeyFile{ "/tmp/public.pem" };
		const fs::path privateKeyFile{ "/tmp/private.pem" };
		CreateKey( publicKeyFile, privateKeyFile );
		auto [modulus2, exponent2] = GetModulusExponent( publicKeyFile );
		vector<unsigned char> modulus = modulus2;
		vector<unsigned char> exponent = exponent2;
		constexpr uint mdlen = SHA256_DIGEST_LENGTH;
		unsigned char md[mdlen];
		auto pDigest = SHA256( (unsigned char*)headerPayload.data(), headerPayload.size(), md ); CHECK_NULL( pDigest );
		KeyPtr pKey{ Internal::ReadPrivateKey(privateKeyFile) };
		CtxPtr ctx{ NewCtx(pKey) };
		CALL( EVP_PKEY_sign_init(ctx.get()) );
		CALL( EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) );	
		CALL( EVP_PKEY_CTX_set_signature_md( ctx.get(), EVP_sha256()) );
		uint siglen;
		CALL( EVP_PKEY_sign(ctx.get(), nullptr, &siglen, pDigest, mdlen) );
		string signature; signature.resize( siglen );
		CALL( EVP_PKEY_sign(ctx.get(), (unsigned char*)signature.data(), &siglen, pDigest, mdlen) );
		auto x = Ssl::Encode64( signature );
		Verify( modulus, exponent, headerPayload, signature );
	}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	α GetModulusExponent( fs::path publicKey )ε->tuple<vector<unsigned char>,vector<unsigned char>>{
		KeyPtr pKey{ Internal::ReadPublicKey(publicKey) };
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