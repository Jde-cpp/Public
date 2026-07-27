#include <jde/fwk/crypto/TrustStore.h>
#ifdef _WIN32
	#include <windows.h>
	#include <wincrypt.h>
#endif
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include "OpenSslInternal.h"
#include <jde/fwk/crypto/OpenSsl.h>

#define let const auto

namespace Jde::Crypto{
	[[maybe_unused]] constexpr ELogTags _tags{ ELogTags::Crypto };//WARN only fires on the linux path.
	using namespace Internal;

	Ω parseDer( std::span<const byte> der, SL sl )ε->X509Ptr{
		const unsigned char* p = (const unsigned char*)der.data();
		X509Ptr cert{ ::d2i_X509(nullptr, &p, (long)der.size()), ::X509_free };
		THROW_IFX( !cert, OpenSslException(sl, 0, "d2i_X509 - {}", OpenSslException::CurrentError()) );
		return cert;
	}

	TrustStore::TrustStore( bool loadOsStore )ε:
		_store{ ::X509_STORE_new(), ::X509_STORE_free }{
		CHECK_NULL( _store );
		if( !loadOsStore )
			return;
#ifdef _WIN32
		//OpenSSL's winstore loader only serves by-subject searches (providers/implementations/storemgmt/winstore_store.c) - enumerate ROOT directly for an eager snapshot.
		let winStore = ::CertOpenSystemStoreW( 0, L"ROOT" );
		THROW_IFX( !winStore, OpenSslException(SRCE_CUR, (uint32)::GetLastError(), "CertOpenSystemStoreW(ROOT) failed") );
		for( PCCERT_CONTEXT winCert{}; (winCert = ::CertEnumCertificatesInStore(winStore, winCert))!=nullptr; ){
			const unsigned char* p = winCert->pbCertEncoded;
			if( X509Ptr cert{ ::d2i_X509(nullptr, &p, (long)winCert->cbCertEncoded), ::X509_free }; cert )
				::X509_STORE_add_cert( _store.get(), cert.get() );//skip unparseable entries; duplicates return success.
		}
		::CertCloseStore( winStore, 0 );
#else
		CALL( ::X509_STORE_set_default_paths(_store.get()) );
		if( CertCount()==0 && !std::getenv("SSL_CERT_FILE") && !std::getenv("SSL_CERT_DIR") ){
			for( const char* bundle : {"/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt"} ){
				if( fs::exists(bundle) && ::X509_STORE_load_locations(_store.get(), bundle, nullptr)==1 )
					break;
			}
			if( CertCount()==0 )
				WARN( "OS trust store is empty - set SSL_CERT_FILE/SSL_CERT_DIR if OpenSSL's default paths are wrong." );
		}
#endif
	}

	α TrustStore::AddCertificate( std::span<const byte> der, SL sl )ε->void{
		let cert = parseDer( der, sl );
		CALLSL( ::X509_STORE_add_cert(_store.get(), cert.get()) );//up-refs the cert.
	}

	α TrustStore::Verify( std::span<const byte> der, SL sl )Ε->void{
		let cert = parseDer( der, sl );
		up<X509_STORE_CTX, decltype(&::X509_STORE_CTX_free)> ctx{ ::X509_STORE_CTX_new(), ::X509_STORE_CTX_free };
		CHECK_NULL( ctx );
		CALLSL( ::X509_STORE_CTX_init(ctx.get(), _store.get(), cert.get(), nullptr) );
		if( ::X509_verify_cert(ctx.get())!=1 ){//verify error lives in the ctx, not the ERR queue.
			let err = ::X509_STORE_CTX_get_error( ctx.get() );
			throw OpenSslException{ sl, (uint32)err, "Certificate not trusted: {} (code={}, depth={})", ::X509_verify_cert_error_string(err), err, ::X509_STORE_CTX_get_error_depth(ctx.get()) };
		}
	}

	α TrustStore::IsTrusted( std::span<const byte> der )Ι->bool{
		try{
			Verify( der );
			return true;
		}catch( const std::exception& ){
			return false;
		}
	}

	α TrustStore::CertCount()Ι->uint{
		return (uint)sk_X509_OBJECT_num( ::X509_STORE_get0_objects(_store.get()) );
	}
}
