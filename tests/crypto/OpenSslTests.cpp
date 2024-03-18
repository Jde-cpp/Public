#include <jde/crypto/OpenSsl.h>
#include "../../src/crypto/OpenSslInternal.h"
#include "../../../Ssl/source/Ssl.h"

#define var const auto
namespace Jde::Crypto{
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
		static α CreateCertificate()ε->void;
		
		constexpr static string HeaderPayload{ "secret stuff" };
		constexpr static string PublicKeyFile{ "/tmp/public.pem" };
		constexpr static sv PrivateKeyFile{ "/tmp/private.pem" };
		constexpr static string CertificateFile{ "/tmp/cert.pem" };
	};
	
	α OpenSslTests::SetUpTestCase()->void{
		var clear = Settings::Get<bool>( "cryptoTests/clear" ).value_or( false );
		INFO( "clear={}", clear );
		//LOG( ELogLevel::Information, _logTag, "clear={}", clear );
		//Logging::Log( Logging::MessageBase("clear={}", ELogLevel::Information, __FILE__, __func__, __LINE__), _logTag, true )
/*		if( clear || (!fs::exists(PublicKeyFile) || !fs::exists(PrivateKeyFile)) ){
			Crypto::CreateKey( PublicKeyFile, PrivateKeyFile );
			INFO( "Created keys {} {}", PublicKeyFile, PrivateKeyFile );
		}*/
		if( clear || !fs::exists(CertificateFile) ){
			CreateCertificate();
			INFO( "Created certificate {}", CertificateFile );
		}
	}
	
	TEST_F( OpenSslTests, Main ){
		auto [modulus2, exponent2] = GetModulusExponent( PublicKeyFile );
		vector<unsigned char> modulus = modulus2;
		vector<unsigned char> exponent = exponent2;
		constexpr uint mdlen = SHA256_DIGEST_LENGTH;
		unsigned char md[mdlen];
		auto pDigest = SHA256( (unsigned char*)HeaderPayload.data(), HeaderPayload.size(), md ); CHECK_NULL( pDigest );
		KeyPtr pKey{ Internal::ReadPrivateKey(PrivateKeyFile) };
		CtxPtr ctx{ NewCtx(pKey) };
		CALL( EVP_PKEY_sign_init(ctx.get()) );
		CALL( EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING) );	
		CALL( EVP_PKEY_CTX_set_signature_md( ctx.get(), EVP_sha256()) );
		uint siglen;
		CALL( EVP_PKEY_sign(ctx.get(), nullptr, &siglen, pDigest, mdlen) );
		string signature; signature.resize( siglen );
		CALL( EVP_PKEY_sign(ctx.get(), (unsigned char*)signature.data(), &siglen, pDigest, mdlen) );
		auto x = Ssl::Encode64( signature );
		Verify( modulus, exponent, HeaderPayload, signature );
	}

	TEST_F( OpenSslTests, Certificate ){
		ReadCertificate( CertificateFile );
		//ReadCertificate( "/tmp/cert2.pem" );
	}
	TEST_F( OpenSslTests, PrivateKey ){
		Crypto::ReadPrivateKey( PrivateKeyFile );
	}

	α OpenSslTests::CreateCertificate()ε->void{
		X509Ptr cert{ ::X509_new(), ::X509_free };
		auto pCert = cert.get();

		::ASN1_INTEGER_set( ::X509_get_serialNumber(pCert), 1 );
		::X509_set_version( pCert, 2 );//X509v3
		::X509_gmtime_adj( ::X509_get_notBefore(pCert), 0 );
		::X509_gmtime_adj( ::X509_get_notAfter(pCert), 365*24*60*60 );
		var privateKey{ Internal::ReadPrivateKey(PrivateKeyFile) };
		::X509_set_pubkey( pCert, privateKey.get() );//Send the private key

		var name{ ::X509_get_subject_name(pCert) };
		::X509_NAME_add_entry_by_txt( name, "C",  MBSTRING_ASC, (unsigned char*)"US", -1, -1, 0 );
		::X509_NAME_add_entry_by_txt( name, "O",  MBSTRING_ASC, (unsigned char*)"jde-cpp", -1, -1, 0 );
		::X509_NAME_add_entry_by_txt( name, "CN", MBSTRING_ASC, (unsigned char*)"localhost", -1, -1, 0 );

 		//auto subject_alt_name = "URI:urn:open62541.server.application";
    // X509V3_CTX ctx{};
    // X509V3_set_ctx_nodb(&ctx);
		// X509V3_set_ctx(&ctx, pCert, pCert, nullptr, nullptr, 0);
		// auto ex = X509V3_EXT_conf_nid( nullptr, &ctx, NID_subject_alt_name, subject_alt_name ); CHECK_NULL( ex );
		// X509_add_ext(pCert, ex, -1);

		// using OctPtr = up<ASN1_OCTET_STRING, decltype(&::ASN1_OCTET_STRING_free)>;
    // auto subject_alt_name_ASN1 = OctPtr{ ASN1_OCTET_STRING_new(), ::ASN1_OCTET_STRING_free };
    // CALL( ASN1_OCTET_STRING_set(subject_alt_name_ASN1.get(), (unsigned char*)subject_alt_name, strlen(subject_alt_name)) );
		// 
    // ExtPtr extension_san{ X509_EXTENSION_create_by_NID(nullptr, NID_subject_alt_name, 0, subject_alt_name_ASN1.get()), ::X509_EXTENSION_free }; CHECK_NULL( extension_san );
    // CALL( X509_add_ext(pCert, extension_san.get(), -1) );

		auto add_x509V3ext = [&](int nid, const char* value) {
    	X509V3_CTX ctx;
    	X509V3_set_ctx_nodb(&ctx);
    	X509V3_set_ctx(&ctx, pCert, pCert, nullptr, nullptr, 0);
			using ExtPtr = up<X509_EXTENSION, decltype(&::X509_EXTENSION_free)>;
    	ExtPtr ex{ X509V3_EXT_conf_nid(nullptr, &ctx, nid, value), ::X509_EXTENSION_free }; CHECK_NULL(ex);
			X509_add_ext(pCert, ex.get(), -1);
		};
		//add_x509V3ext(NID_subject_key_identifier, "hash");
		//add_x509V3ext(NID_basic_constraints, "CA:FALSE");
		add_x509V3ext(NID_subject_alt_name, "URI:urn:open62541.server.application");
		

		//X509_EXTENSION* cert_ex = X509V3_EXT_conf_nid( nullptr, nullptr, NID_subject_alt_name, "urn:open62541.server.application" );  CHECK_NULL( cert_ex );
		//X509_add_ext( pCert, cert_ex, -1 );
		//::X509V3_EXT_conf_nid( nullptr, pCert, NID_subject_alt_name, (char*)"urn:open62541.server.application" );

		::X509_set_issuer_name( pCert, name );
		::X509_sign( pCert, privateKey.get(), ::EVP_sha256() );

		BioPtr file{ BIO_new_file(CertificateFile.c_str(), "w"), ::BIO_free };
		CALL( PEM_write_bio_X509(file.get(), pCert) );
	}
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	α OpenSslTests::GetModulusExponent( fs::path publicKey )ε->tuple<vector<unsigned char>,vector<unsigned char>>{
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