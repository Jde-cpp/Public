#include "OpenSslInternal.h"
#include <jde/crypto/OpenSsl.h>

namespace Jde::Crypto{
	template<typename T, typename D>
	α MakeHandle( T* p, D deleter, SRCE )ε{
		THROW_IFX( !p, Crypto::OpenSslException("MakeHandle - p is null - {}", 0, sl, Crypto::OpenSslException::CurrentError()) );
		return up<T,D>{ p, deleter };
	}

	using namespace Jde::Crypto::Internal;
	α File( const fs::path& path, const char* rw )ε->BioPtr{
		auto p = BIO_new_file(path.string().c_str(), rw); CHECK_NULL( p );
		return BioPtr{ p, ::BIO_free };
	}
	α Internal::ReadFile( const fs::path& path )ε->BioPtr{ return File( path, "r" ); }
	α Internal::ReadPublicKey( const fs::path& path )ε->KeyPtr{ 
		EVP_PKEY* pkey = ::PEM_read_bio_PUBKEY( ReadFile(path).get(), nullptr, nullptr, nullptr ); CHECK_NULL( pkey );
		return { pkey, ::EVP_PKEY_free };
	}
	α Internal::ReadPrivateKey( const fs::path& path, str passcode )ε->KeyPtr{ 
		auto p = ReadFile(path);
		EVP_PKEY* pkey = PEM_read_bio_PrivateKey( p.get(), nullptr, nullptr, (void*)passcode.c_str() ); CHECK_NULL( pkey );
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