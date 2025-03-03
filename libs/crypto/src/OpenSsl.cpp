#include <jde/crypto/OpenSsl.h>
#include "OpenSslInternal.h"
#include <jde/framework/str.h>

#define let const auto

namespace Jde{
	namespace Crypto{
		auto _logTag = Logging::Tag( "crypto" );
		α OpenSslException::CurrentError()ι->string{ char b[120]; ERR_error_string( ERR_get_error(), b ); return {b}; }
	}
	using namespace Jde::Crypto::Internal;

	α Crypto::CalcMd5( sv content )ε->MD5{
		auto ctx = NewMDCtx();
		CALL( EVP_DigestInit_ex(ctx.get(), EVP_md5(), nullptr) );
		CALL(	EVP_DigestUpdate(ctx.get(), content.data(), content.size()) );
		array<byte,EVP_MAX_MD_SIZE> buffer; unsigned int size;
		CALL(	EVP_DigestFinal_ex(ctx.get(), (unsigned char*)buffer.data(), &size) );
		ASSERT( size==16 );
		MD5 md5;
		std::copy( buffer.begin(), buffer.begin()+16, md5.begin() );
		return md5;
	}

	α Crypto::CreateKeyCertificate( const CryptoSettings& settings )ε->void{
		CreateKey( settings.PublicKeyPath, settings.PrivateKeyPath, settings.Passcode );
		CreateCertificate( settings.CertPath, settings.PrivateKeyPath, settings.Passcode, settings.AltName, settings.Company, settings.Country, settings.Domain );
	}
	//https://stackoverflow.com/questions/5927164/how-to-generate-rsa-private-key-using-openssl
	α Crypto::CreateKey( const fs::path& publicKeyPath, const fs::path& privateKeyPath, str& passcode )ε->void{
		auto pctx = NewRsaCtx();
		uint32_t bits = 2048;
		uint32_t publicExponent = 65537;
		OSSL_PARAM params[3]{ OSSL_PARAM_construct_uint("bits", &bits), OSSL_PARAM_construct_uint("e", &publicExponent),  OSSL_PARAM_construct_end() };
		EVP_PKEY_CTX_set_params(pctx.get(), params);
		EVP_PKEY* key{};
		EVP_PKEY_generate(pctx.get(), &key); CHECK_NULL( key );
		KeyPtr pKey( key, ::EVP_PKEY_free );

		BioPtr publicBio{ BIO_new_file( publicKeyPath.string().c_str(), "w"), ::BIO_free }; CHECK_NULL( publicBio );
		CALL( PEM_write_bio_PUBKEY(publicBio.get(), pKey.get()) );
		Internal::WritePrivateKey( privateKeyPath, move(pKey), passcode );
	}

	α Crypto::CreateCertificate( fs::path outputFile, fs::path privateKeyFile, str passcode, sv altName, sv company, sv country, sv domain )ε->void {
		X509Ptr cert{ ::X509_new(), ::X509_free };
		auto pCert = cert.get();

		::ASN1_INTEGER_set( ::X509_get_serialNumber(pCert), 1 );
		::X509_set_version( pCert, 2 );//X509v3
		::X509_gmtime_adj( ::X509_get_notBefore(pCert), 0 );
		::X509_gmtime_adj( ::X509_get_notAfter(pCert), 365 * 24 * 60 * 60 );
		let privateKey{ Internal::ReadPrivateKey(privateKeyFile, passcode) };
		::X509_set_pubkey( pCert, privateKey.get() );

		auto add_x509V3ext = [&]( int nid, const char* value )ε{
			X509V3_CTX ctx;
			X509V3_set_ctx_nodb( &ctx );
			X509V3_set_ctx( &ctx, pCert, pCert, nullptr, nullptr, 0 );
			using ExtPtr = up<X509_EXTENSION, decltype(&::X509_EXTENSION_free)>;
			ExtPtr ex{ X509V3_EXT_conf_nid(nullptr, &ctx, nid, value), ::X509_EXTENSION_free }; CHECK_NULL( ex );
			X509_add_ext( pCert, ex.get(), -1 );
		};
		if( altName.size() )
			add_x509V3ext( NID_subject_alt_name, string{altName}.c_str() );

		auto name{ ::X509_get_subject_name(pCert) };
		::X509_NAME_add_entry_by_txt( name, "C", MBSTRING_ASC, (unsigned char*)string{country}.c_str(), -1, -1, 0 );
		::X509_NAME_add_entry_by_txt( name, "O", MBSTRING_ASC, (unsigned char*)string{company}.c_str(), -1, -1, 0 );
		::X509_NAME_add_entry_by_txt( name, "CN", MBSTRING_ASC, (unsigned char*)string{domain}.c_str(), -1, -1, 0 );
		::X509_set_issuer_name( pCert, name );
		::X509_sign( pCert, privateKey.get(), ::EVP_sha256() );

		BioPtr file{ BIO_new_file(outputFile.string().c_str(), "w"), ::BIO_free };
		CALL( PEM_write_bio_X509(file.get(), pCert) );
	}
	α RsaPemFromModExp( Crypto::Modulus modulus, const Crypto::Exponent& exponent )ε->KeyPtr;

	α Crypto::Fingerprint( const Modulus& modulus, const Exponent& exponent )ι->MD5{
		//openssl pkey -pubin -in server.pem -outform DER | openssl dgst -md5 -c
		auto key = RsaPemFromModExp( modulus, exponent );
		array<byte,1040> buffer; uint len;
		EVP_PKEY_get_raw_public_key( key.get(), (unsigned char*)buffer.data(), &len );
		MD5 md5;
		std::copy( buffer.begin(), buffer.begin()+16, md5.begin() );
		return md5;
	}

	α Crypto::ModulusExponent( const fs::path& publicKey )ε->tuple<Modulus,vector<unsigned char>>{
		KeyPtr pKey{ Internal::ReadPublicKey(publicKey) };
		BIGNUM* n{}, *e{};
		CALL( EVP_PKEY_get_bn_param(pKey.get(), "e", &e) );
		CALL( EVP_PKEY_get_bn_param(pKey.get(), "n", &n) );
		BNPtr pN( n, ::BN_free );
		BNPtr pE( e, ::BN_free );
		Modulus modulus( BN_num_bytes(n) );
		BN_bn2bin( pN.get(), modulus.data() );
		vector<unsigned char> exponent( BN_num_bytes(e) );
		BN_bn2bin( pE.get(), exponent.data() );
		return { modulus, exponent };
	}

	α Crypto::RsaSign( str content, const fs::path& privateKeyFile )ε->Signature{
		array<unsigned char, SHA256_DIGEST_LENGTH> md;
		auto pDigest = SHA256( (const unsigned char*)content.data(), content.size(), md.data() ); CHECK_NULL( pDigest );
		KeyPtr pKey{ Internal::ReadPrivateKey(privateKeyFile) };
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
	α RsaPemFromModExp( Crypto::Modulus modulus, const Crypto::Exponent& exponent )ε->KeyPtr{//this changes the modulus.
		//uint32_t bigEndian{ __builtin_bswap32(exponent) };
//		array<unsigned char,3> bigEndianBytes{ 1, 0, 1 };
		OSSL_PARAM params[]{
			OSSL_PARAM_construct_BN( "n", (unsigned char*)modulus.data(), modulus.size() ),
			OSSL_PARAM_construct_BN( "e", (unsigned char*)exponent.data(), exponent.size() ),
			OSSL_PARAM_construct_end() };

		auto pMod = ToBigNum( modulus );
		auto pExp = ToBigNum( exponent );//above seems to allocate space, this sets.
		//sets return size.
		OSSL_PARAM_set_BN( &params[0], pMod.get() );
		OSSL_PARAM_set_BN( &params[1], pExp.get() );

    EVP_PKEY* key{};
		auto pctx = NewRsaCtx();
		CALL( EVP_PKEY_fromdata_init(pctx.get()) );
		CALL( EVP_PKEY_fromdata(pctx.get(), &key, EVP_PKEY_PUBLIC_KEY, params) );
		return { key, ::EVP_PKEY_free };
	}

	α Crypto::Verify( const Modulus& modulus, const Exponent& exponent, str decrypted, const Signature& signature )ε->void{
		using ContextPtr = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;
		ContextPtr pCtx{ EVP_MD_CTX_create(), ::EVP_MD_CTX_free };

		let pMd = EVP_get_digestbyname( "SHA256" ); CHECK_NULL( pMd ); // do not need to be freed with EVP_MD_free
		CALL( EVP_VerifyInit_ex( pCtx.get(), pMd, nullptr) );
		CALL( EVP_VerifyUpdate( pCtx.get(), decrypted.c_str(), decrypted.size()) );
		let pKey = RsaPemFromModExp( modulus, exponent );
		CALL( EVP_VerifyFinal(pCtx.get(), (const unsigned char*)signature.data(), (int)signature.size(), pKey.get()) );
	}

	α Crypto::ReadCertificate( const fs::path& certificate )ε->vector<byte>{
		X509Ptr cert{ PEM_read_bio_X509(ReadFile(certificate).get(), nullptr, 0, nullptr), ::X509_free };  CHECK_NULL( cert );

		auto len = i2d_X509( cert.get(), nullptr ); THROW_IFX( len<=0, Crypto::OpenSslException("i2d_X509 - {}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) );
		vector<byte> y( len );
		unsigned char* p = (unsigned char*)y.data();
		len = i2d_X509( cert.get(), &p ); THROW_IFX( len<=0, Crypto::OpenSslException("i2d_X509 - {}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) );
		int crit{};
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

	α Crypto::WritePrivateKey( const fs::path& path, vector<byte>&& privateKey, str passcode )ε->void{
		auto bio = Internal::ToBio( move(privateKey) );
		KeyPtr pkey{ ::d2i_PrivateKey_bio(bio.get(), nullptr), ::EVP_PKEY_free }; CHECK_NULL( pkey );
		Internal::WritePrivateKey( path, move(pkey), passcode );
	}

	α Crypto::WriteCertificate( const fs::path& path, vector<byte>&& certificate )ε->void{
		BioPtr mem{ Internal::ToBio(move(certificate)) };
		X509Ptr cert{ d2i_X509_bio(mem.get(), nullptr), ::X509_free };  CHECK_NULL( cert );
		BioPtr file{ File(path, true) };
		CALL( ::PEM_write_bio_X509(file.get(), cert.get()) );
	}
}