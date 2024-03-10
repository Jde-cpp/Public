#include <jde/crypto/OpenSsl.h>
#include "../../../Ssl/source/Ssl.h"

#define var const auto

namespace Jde{
	using namespace Jde::Crypto::Internal;

	#define CALL( call ) if( int rc=call; rc!=1 ) throw Crypto::OpenSslException( "#call - {}", rc, SRCE_CUR, Crypto::OpenSslException::CurrentError() )
	#define CHECK_NULL( p ) THROW_IFX( !p, Crypto::OpenSslException("{}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) )

	template<typename T, typename D>
	α MakeHandle( T* p, D deleter, SRCE )ε{
		THROW_IFX( !p, Crypto::OpenSslException("MakeHandle - p is null - {}", 0, sl, Crypto::OpenSslException::CurrentError()) );
		return up<T,D>{ p, deleter };
	}
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
	namespace Crypto{
		α File( const fs::path& path, const char* rw )ε->BioPtr{
			auto p = BIO_new_file(path.string().c_str(), rw); CHECK_NULL( p );
			return BioPtr{ p, ::BIO_free };
		}
		α Internal::ReadFile( const fs::path& path )ε->BioPtr{ return File( path, "r" ); }
		α Internal::ReadPublicKey( const fs::path& path )ε->KeyPtr{ 
			//EVP_PKEY_print_public(publicBio.get(), pKey.get(), 0, nullptr) ); //Traditional PEM
			EVP_PKEY* pkey = ::PEM_read_bio_PUBKEY( ReadFile(path).get(), nullptr, nullptr, nullptr ); CHECK_NULL( pkey );
			return { pkey, ::EVP_PKEY_free };
		}
		α Internal::ReadPrivateKey( const fs::path& path )ε->KeyPtr{ 
			EVP_PKEY* pkey = PEM_read_bio_PrivateKey( ReadFile(path).get(), nullptr, nullptr, nullptr ); CHECK_NULL( pkey );
			return { pkey, ::EVP_PKEY_free };
		}
		α Internal::NewRsaCtx(SL sl)ε->CtxPtr{ 
			auto pctx = MakeHandle( EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr), EVP_PKEY_CTX_free, sl ); 
			EVP_PKEY_keygen_init( pctx.get() );
			return pctx;
		}
		α Internal::NewCtx( const KeyPtr& key, SL sl )ε->CtxPtr{
			return MakeHandle( EVP_PKEY_CTX_new(key.get(), nullptr), EVP_PKEY_CTX_free, sl );
		}
		α Internal::ToBigNum( const vector<unsigned char>& x )ε->BNPtr{
			auto p = MakeHandle( BN_bin2bn( x.data(), (int)x.size(), nullptr ), ::BN_free, SRCE_CUR ); CHECK_NULL( p.get() );
			return p;
		}
	}
}