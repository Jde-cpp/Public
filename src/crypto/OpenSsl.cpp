#include <jde/crypto/OpenSsl.h>
#include "OpenSslInternal.h"
#include "../../../Ssl/source/Ssl.h"

#define var const auto

namespace Jde{
	namespace Crypto{
		auto _logTag = Logging::Tag( "crypto" );
		α OpenSslException::CurrentError()ι->string{ char b[120]; ERR_error_string( ERR_get_error(), b ); return {b}; }
	}
	using namespace Jde::Crypto::Internal;

	//https://stackoverflow.com/questions/5927164/how-to-generate-rsa-private-key-using-openssl
	α Crypto::CreateKey( const fs::path& publicKeyPath, const fs::path& privateKeyPath )ε->void{
		auto pctx = NewRsaCtx();
		uint32_t bits = 2048;
		uint32_t publicExponent = 65537;
		OSSL_PARAM params[3]{ OSSL_PARAM_construct_uint("bits", &bits), OSSL_PARAM_construct_uint("e", &publicExponent),  OSSL_PARAM_construct_end() };
		EVP_PKEY_CTX_set_params(pctx.get(), params);
		EVP_PKEY* key{};
		EVP_PKEY_generate(pctx.get(), &key); CHECK_NULL( key );
		KeyPtr pKey( key, ::EVP_PKEY_free );

		BioPtr publicBio{ BIO_new_file( publicKeyPath.string().c_str(), "w"), ::BIO_free };
		CALL( PEM_write_bio_PUBKEY(publicBio.get(), pKey.get()) );
		BioPtr privateBio{ BIO_new_file(privateKeyPath.string().c_str(), "w"), ::BIO_free };
		CALL( PEM_write_bio_PrivateKey(privateBio.get(), pKey.get(), nullptr, nullptr, 0, nullptr, nullptr) );
	}

//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	α Crypto::RsaSign( sv value, sv key )ι->string{
		unsigned char buffer[EVP_MAX_MD_SIZE];
		uint32_t len = sizeof(buffer);
		HMAC( EVP_sha1(), key.data(), key.size(), (const unsigned char*)value.data(), value.size(), buffer, &len );
		return Ssl::Encode64( string{buffer, buffer+len} );
	}

	//https://stackoverflow.com/questions/28770426/rsa-public-key-conversion-with-just-modulus
	α RsaPemFromModExp( const vector<unsigned char>& modulus, const vector<unsigned char>& exponent )ε->KeyPtr{
		OSSL_PARAM params[]{ OSSL_PARAM_construct_BN("n", (unsigned char*)modulus.data(), modulus.size()), OSSL_PARAM_construct_BN("e", (unsigned char*)exponent.data(), exponent.size()),  OSSL_PARAM_construct_end() };
		auto pMod = ToBigNum( modulus );
		auto pExp = ToBigNum( exponent );
		//sets return size.
		OSSL_PARAM_set_BN( &params[0], pMod.get() );
		OSSL_PARAM_set_BN( &params[1], pExp.get() );

    EVP_PKEY* key{};
		auto pctx = NewRsaCtx();
		CALL( EVP_PKEY_fromdata_init(pctx.get()) );
		CALL( EVP_PKEY_fromdata(pctx.get(), &key, EVP_PKEY_PUBLIC_KEY, params) );
		return { key, ::EVP_PKEY_free };
	}

	α Crypto::Verify( const vector<unsigned char>& modulus, const vector<unsigned char>& exponent, str decrypted, str signature )ε->void{
		using ContextPtr = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;
		ContextPtr pCtx{ EVP_MD_CTX_create(), ::EVP_MD_CTX_free };
		var pMd = EVP_get_digestbyname( "SHA256" ); CHECK_NULL( pMd ); // do not need to be freed with EVP_MD_free 
		CALL( EVP_VerifyInit_ex( pCtx.get(), pMd, nullptr) );
		CALL( EVP_VerifyUpdate( pCtx.get(), decrypted.c_str(), decrypted.size()) );
		var pKey = RsaPemFromModExp( modulus, exponent );
		CALL( EVP_VerifyFinal(pCtx.get(), (const unsigned char*)signature.c_str(), (int)signature.size(), pKey.get()) );
	}

	α Crypto::ReadCertificate( const fs::path& certificate )ε->vector<byte>{
		X509Ptr cert{ PEM_read_bio_X509(ReadFile(certificate).get(), nullptr, 0, nullptr), ::X509_free };  CHECK_NULL( cert );

		auto len = i2d_X509( cert.get(), nullptr ); THROW_IFX( len<=0, Crypto::OpenSslException("i2d_X509 - {}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) );
		vector<byte> y( len );
		unsigned char* p = (unsigned char*)y.data();
		len = i2d_X509( cert.get(), &p ); THROW_IFX( len<=0, Crypto::OpenSslException("i2d_X509 - {}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) );
		int crit{};
 		GENERAL_NAMES* pNames = (GENERAL_NAMES*) X509_get_ext_d2i( cert.get(), NID_subject_alt_name, &crit, nullptr ); CHECK_NULL(pNames);
		return y;
	}
	α Crypto::ReadPrivateKey( const fs::path& privateKeyPath, str passcode )ε->vector<byte>{
		auto pkey = Internal::ReadPrivateKey( privateKeyPath, passcode );
		auto len = i2d_PrivateKey( pkey.get(), nullptr ); THROW_IFX( len<=0, Crypto::OpenSslException("i2d_PrivateKey - {}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) );
		vector<byte> y( len );
		unsigned char* pTemp = (unsigned char*)y.data();
		len = i2d_PrivateKey( pkey.get(), &pTemp ); THROW_IFX( len<=0, Crypto::OpenSslException("i2d_PrivateKey - {}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) );
		return y;
	}
}