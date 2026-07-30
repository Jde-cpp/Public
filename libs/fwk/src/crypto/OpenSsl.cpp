#include <jde/fwk/crypto/OpenSsl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include <openssl/x509v3.h>
#include "OpenSslInternal.h"
#include <jde/fwk/str.h>
#include <jde/fwk/crypto/CryptoSettings.h>

#define let const auto

namespace Jde{
	using namespace Jde::Crypto::Internal;
	namespace Crypto{
		constexpr ELogTags _tags{ ELogTags::Crypto };
		α OpenSslException::CurrentError()ι->string{ return CurrentError(CurrentErrorCode()); }
		α OpenSslException::CurrentError( uint32 rc )ι->string{ if( !rc ) return "no queued openssl error"; char b[256]; ERR_error_string_n(rc, b, sizeof(b)); return {b}; }//0 would format as 'error:00000000...' - noise masquerading as detail.
		α OpenSslException::CurrentErrorCode()ι->uint32{ return (uint32)ERR_get_error(); }

		//https://stackoverflow.com/questions/1986888/how-to-compute-a-32-bit-fingerprint-of-a-certificate
		α PublicKey::Hash32()Ι->uint32_t{
			auto md5 = CalcMd5( Modulus );
			uint32_t hash = 0;
			for( auto b : md5 )
				hash = ( hash << 8 ) | ( uint32_t )b;
			return hash;
		}
		α PublicKey::ToBytes()ε->vector<byte>{
			return Crypto::ToBytes( *this );
		}

		α PublicKey::ExponentInt()Ι->uint32_t{
			uint32_t exponent{};
			for( let i : Exponent )
				exponent = ( exponent<<8 ) | i;
			return exponent;
		}
		α PublicKey::ModulusHex()Ε->string{
			auto modHex = Str::ToHex( (byte*)Modulus.data(), Modulus.size() );
			THROW_IF( modHex.size() > 1024, "modulus {} is too long. max length: {}", modHex.size(), 1024 );
			return modHex;
		}

	}

	α Crypto::Random( unsigned char* p, uint size )ε->void{
		THROW_IFX( ::RAND_bytes(p, (int)size)!=1, OpenSslException(Ƒ("RAND_bytes({}) failed", size)) );
	}
	α Crypto::CalcMd5( byte* data, uint size )ε->MD5{
		if( size==0 )
			return EmptyStringMd5;
		auto ctx = NewMDCtx();
		CALL( EVP_DigestInit_ex(ctx.get(), EVP_md5(), nullptr) );
		CALL( 	EVP_DigestUpdate(ctx.get(), data, size) );
		MD5 md5;
		unsigned int outputSize;
		CALL( 	EVP_DigestFinal_ex(ctx.get(), md5.data(), &outputSize) );
		return md5;
	}

	α Crypto::CreateKeyCertificate( const CryptoSettings& settings, SL sl )ε->void{
		CreateKey( settings, sl );
		CreateCertificate( settings, sl );
	}

	α Crypto::EnsureKeyCertificate( const CryptoSettings& settings, SL sl )ε->void{
		if( !fs::exists(settings.PrivateKey.Path) ){
			settings.CreateDirectories();
			CreateKeyCertificate( settings, sl );
			return;
		}
		try{
			Certificate{ ReadCertificate(settings.Certificate.Path), sl }.Log( Ƒ("Read Certificate at {}", settings.Certificate.Path.string()), sl );
		}
		catch( const std::exception& e ){//missing or unreadable cert (interrupted write, partial restore) - re-issue on the existing key rather than fail every start; the modulus is unchanged, so enrolled users are unaffected.
			WARN( "Re-issuing certificate '{}': {}", settings.Certificate.Path.string(), e.what() );
			settings.CreateDirectories();
			CreateCertificate( settings, sl );
		}
	}

	//https://stackoverflow.com/questions/5927164/how-to-generate-rsa-private-key-using-openssl
	α Crypto::CreateKey( const CryptoSettings& settings, SL sl )ε->void{
		auto pctx = NewRsaCtx();
		uint32_t bits = 2048;
		uint32_t publicExponent = 65537;
		OSSL_PARAM params[3]{ OSSL_PARAM_construct_uint("bits", &bits), OSSL_PARAM_construct_uint("e", &publicExponent),  OSSL_PARAM_construct_end() };
		EVP_PKEY_CTX_set_params( pctx.get(), params );
		EVP_PKEY* key{};
		EVP_PKEY_generate( pctx.get(), &key ); CHECK_NULL( key );
		KeyPtr pKey( key, ::EVP_PKEY_free );

		BioPtr publicBio{ BIO_new_file(settings.PublicKey.Path.string().c_str(), "w"), ::BIO_free }; CHECK_NULL( publicBio );
		INFO( "Created public key at {}", settings.PublicKey.Path.string() );
		CALL( PEM_write_bio_PUBKEY(publicBio.get(), pKey.get()) );
		Internal::WritePrivateKey( settings.PrivateKey.Path, move(pKey), settings.PrivateKey.Passcode );
	}

	α Crypto::CreateCertificate( const CryptoSettings& settings, SL sl )ε->void{
		X509Ptr cert{ ::X509_new(), ::X509_free };
		auto pCert = cert.get();

		::ASN1_INTEGER_set( ::X509_get_serialNumber(pCert), 1 );
		::X509_set_version( pCert, 2 );//X509v3
		::X509_gmtime_adj( ::X509_get_notBefore(pCert), 0 );
		::X509_gmtime_adj( ::X509_get_notAfter(pCert), 365 * 24 * 60 * 60 );
		let privateKey{ Internal::ReadPrivateKey(settings.PrivateKey.Path, settings.PrivateKey.Passcode, sl) };
		::X509_set_pubkey( pCert, privateKey.get() );

		auto add_x509V3ext = [&]( int nid, const char* value )ε{
			X509V3_CTX ctx;
			X509V3_set_ctx_nodb( &ctx );
			X509V3_set_ctx( &ctx, pCert, pCert, nullptr, nullptr, 0 );
			using ExtPtr = up<X509_EXTENSION, decltype( &::X509_EXTENSION_free )>;
			ExtPtr ex{ X509V3_EXT_conf_nid(nullptr, &ctx, nid, value), ::X509_EXTENSION_free }; CHECK_NULL( ex );
			X509_add_ext( pCert, ex.get(), -1 );
		};
		let& certSettings = settings.Certificate;
		if( certSettings.SubjectAltName.size() )
			add_x509V3ext( NID_subject_alt_name, string{certSettings.SubjectAltName}.c_str() );

		auto name{ ::X509_get_subject_name(pCert) };
		if( !certSettings.Country.empty() )
			::X509_NAME_add_entry_by_txt( name, "C", MBSTRING_ASC, (unsigned char*)string{certSettings.Country}.c_str(), -1, -1, 0 ); //country code
		if( !certSettings.Company.empty() )
			::X509_NAME_add_entry_by_txt( name, "O", MBSTRING_ASC, (unsigned char*)string{certSettings.Company}.c_str(), -1, -1, 0 ); //organization name
		if( !certSettings.CommonName.empty() )
			::X509_NAME_add_entry_by_txt( name, "CN", MBSTRING_ASC, (unsigned char*)string{certSettings.CommonName}.c_str(), -1, -1, 0 ); //common name
		::X509_set_issuer_name( pCert, name );
		::X509_sign( pCert, privateKey.get(), ::EVP_sha256() );

		let path = certSettings.Path;
		BioPtr file{ BIO_new_file(path.string().c_str(), "w"), ::BIO_free }; CHECK_NULL( file );
		CALL( PEM_write_bio_X509(file.get(), pCert) );
		file = nullptr; //write
		Certificate{ ReadCertificate(path), sl }.Log( Ƒ("Created Certificate at {}", path.string()) );
	}

	Ω toPublicKey( KeyPtr&& key, SL sl )ε->Crypto::PublicKey{
		BIGNUM* n{}, *e{};
		CALLSL( EVP_PKEY_get_bn_param(key.get(), "n", &n) );
		CALLSL( EVP_PKEY_get_bn_param(key.get(), "e", &e) );
		BNPtr pN( n, ::BN_free );
		BNPtr pE( e, ::BN_free );
		Crypto::Modulus modulus( BN_num_bytes(n) );
		BN_bn2bin( pN.get(), modulus.data() );
		Crypto::Exponent exponent( BN_num_bytes(e) );
		BN_bn2bin( pE.get(), exponent.data() );
		return { move(modulus), move(exponent) };
	}
	α Crypto::ExtractPublicKey( std::span<byte> certificate, SL sl )ε->PublicKey{
		X509Ptr cert{ ::d2i_X509_bio(ToBio(certificate, sl).get(), nullptr), ::X509_free }; CHECK_NULL( cert.get() );
		KeyPtr key{ X509_get_pubkey(cert.get()), ::EVP_PKEY_free }; CHECK_NULL( key.get() );
		return toPublicKey( move(key), sl );
	}

	Ω rsaPemFromModExp( Crypto::Modulus modulus, const Crypto::Exponent& exponent, SRCE )ε->KeyPtr;

	α Crypto::Fingerprint( const PublicKey& pubKey, SL sl )ε->MD5{
		let bytes = ToBytes( pubKey, sl );
		return CalcMd5( bytes );
	}
	α Crypto::ReadPublicKey( const fs::path& publicKey, SL sl )ε->PublicKey{
		return toPublicKey( Internal::ReadPublicKey(publicKey, sl), sl );
	}
	α Crypto::ToBytes( const PublicKey& modExp, SL sl )ε->vector<byte>{
		let key = rsaPemFromModExp( modExp.Modulus, modExp.Exponent, sl );
		auto len = i2d_PUBKEY( key.get(), nullptr );
		vector<byte> y( len );
		unsigned char* p = ( unsigned char* )y.data();
		len = i2d_PUBKEY( key.get(), &p ); THROW_IFX( len<=0, OpenSslException("i2d_PUBKEY failed") );
		return y;
	}

	α Crypto::RsaSign( str content, const fs::path& privateKeyFile, str passcode, SL sl )ε->Signature{
		array<unsigned char, SHA256_DIGEST_LENGTH> md;
		auto pDigest = SHA256( (const unsigned char*)content.data(), content.size(), md.data() ); CHECK_NULL( pDigest );
		KeyPtr pKey{ Internal::ReadPrivateKey(privateKeyFile, passcode) };
		CtxPtr ctx{ NewCtx(pKey) };
		CALL( EVP_PKEY_sign_init(ctx.get()) );
		CALL( EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) );
		CALL( EVP_PKEY_CTX_set_signature_md(ctx.get(), EVP_sha256()) );
		uint siglen;
		CALL( EVP_PKEY_sign(ctx.get(), nullptr, &siglen, pDigest, md.size()) );
		Signature signature( siglen );
		CALL( EVP_PKEY_sign(ctx.get(), (unsigned char*)signature.data(), &siglen, pDigest, md.size()) );
		return signature;
	}

	//https://stackoverflow.com/questions/28770426/rsa-public-key-conversion-with-just-modulus
	α rsaPemFromModExp( Crypto::Modulus modulus, const Crypto::Exponent& exponent, SL sl )ε->KeyPtr{ //this changes the modulus.
		OSSL_PARAM params[]{
			OSSL_PARAM_construct_BN( "n", (unsigned char*)modulus.data(), modulus.size() ),
			OSSL_PARAM_construct_BN( "e", (unsigned char*)exponent.data(), exponent.size() ),
			OSSL_PARAM_construct_end() };

		auto pMod = ToBigNum( modulus, sl );
		auto pExp = ToBigNum( exponent, sl );//above seems to allocate space, this sets.
		//sets return size.
		OSSL_PARAM_set_BN( &params[0], pMod.get() );
		OSSL_PARAM_set_BN( &params[1], pExp.get() );

    EVP_PKEY* key{};
		auto pctx = NewRsaCtx();
		CALLSL( EVP_PKEY_fromdata_init(pctx.get()) );
		CALLSL( EVP_PKEY_fromdata(pctx.get(), &key, EVP_PKEY_PUBLIC_KEY, params) );
		return { key, ::EVP_PKEY_free };
	}

	α Crypto::Verify( const PublicKey& certificate, str decrypted, const Signature& signature, SL sl )ε->void{
		THROW_IF( certificate.Modulus.size() == 0, "certificate.Modulus.size() == 0" );
		THROW_IF( certificate.Exponent.size() == 0, "certificate.Exponent.size() == 0" );
		using ContextPtr = std::unique_ptr<EVP_MD_CTX, decltype( &::EVP_MD_CTX_free )>;
		ContextPtr ctx{ EVP_MD_CTX_create(), ::EVP_MD_CTX_free };

		let md = EVP_get_digestbyname( "SHA256" ); CHECK_NULL( md ); // do not need to be freed with EVP_MD_free
		CALL( EVP_VerifyInit_ex(ctx.get(), md, nullptr) );
		CALL( EVP_VerifyUpdate(ctx.get(), decrypted.c_str(), decrypted.size()) );
		let key = rsaPemFromModExp( certificate.Modulus, certificate.Exponent );
		let rc = EVP_VerifyFinal( ctx.get(), (const unsigned char*)signature.data(), (int)signature.size(), key.get() );
		if( rc!=1 )//0 is a semantic outcome (signature mismatch - an expected auth event), negative a malfunction.
			throw OpenSslException{ rc==0 ? string{"Signature verification failed."} : Ƒ("EVP_VerifyFinal -> {}", rc), sl };
	}

	α Crypto::ReadCertificate( const fs::path& certificate, SL sl )ε->vector<byte>{
		X509Ptr cert{ PEM_read_bio_X509(Internal::ReadFile(certificate, sl).get(), nullptr, 0, nullptr), ::X509_free };  CHECK_NULL( cert );

		auto len = i2d_X509( cert.get(), nullptr ); THROW_IFX( len<=0, OpenSslException("i2d_X509 failed") );
		vector<byte> y( len );
		unsigned char* p = ( unsigned char* )y.data();
		len = i2d_X509( cert.get(), &p ); THROW_IFX( len<=0, OpenSslException("i2d_X509 failed") );
		return y;
	}
	α Crypto::ReadPrivateKey( const Crypto::PrivateKeySettings& settings )ε->vector<byte>{
		auto pkey = Internal::ReadPrivateKey( settings.Path, settings.Passcode );
		auto len = i2d_PrivateKey( pkey.get(), nullptr ); THROW_IFX( len<=0, OpenSslException("i2d_PrivateKey failed") );
		vector<byte> y( len );
		unsigned char* pTemp = ( unsigned char* )y.data();
		len = i2d_PrivateKey( pkey.get(), &pTemp ); THROW_IFX( len<=0, OpenSslException("i2d_PrivateKey failed") );
		return y;
	}

	α Crypto::WriteCertificate( const fs::path& path, vector<byte>&& certificate, SL sl )ε->void{
		BioPtr mem{ Internal::ToBio(certificate, sl) };
		X509Ptr cert{ ::d2i_X509_bio(mem.get(), nullptr), ::X509_free };  CHECK_NULL( cert );
		BioPtr file{ Internal::File(path, true, sl) };
		CALL( ::PEM_write_bio_X509(file.get(), cert.get()) );
	}

	α Crypto::WritePrivateKey( const fs::path& path, vector<byte>&& privateKey, str passcode, SL sl )ε->void{
		auto bio = Internal::ToBio( privateKey, sl );
		KeyPtr pkey{ ::d2i_PrivateKey_bio(bio.get(), nullptr), ::EVP_PKEY_free }; CHECK_NULL( pkey );
		Internal::WritePrivateKey( path, move(pkey), passcode, sl );
	}
}