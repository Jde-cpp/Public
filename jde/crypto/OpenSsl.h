#pragma once
#include "exports.h"

#define Φ ΓC auto

namespace Jde::Crypto{
	Φ CreateKey( const fs::path& publicKeyPath, const fs::path& privateKeyPath )ε->void;
	Φ RsaSign( sv value, sv key )ι->string;
	Φ Verify( const vector<unsigned char>& modulus, const vector<unsigned char>& exponent, str decrypted, str signature )ε->void;
	struct OpenSslException final : Exception{
		template<class... Args>
		OpenSslException( fmt::format_string<Args...> fmt, uint rc, SL sl, Args&&... args )ι:
			Exception{ sl, ELogLevel::Warning, rc, fmt, std::forward<Args>(args)... }
		{}
		Ω CurrentError()ι->string{ char b[120]; ERR_error_string( ERR_get_error(), b ); return {b}; }
  };
	
//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"	
	namespace Internal{
		using BioPtr = up<BIO, decltype(&::BIO_free)>;
		using KeyPtr = up<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
		using CtxPtr = up<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;
		using BNPtr = up<BIGNUM, decltype(&::BN_free)>;
		//using RsaPtr = up<RSA, decltype(&::RSA_free)>;
		//using CRsaPtr = up<const RSA, decltype(&::RSA_free)>;
		α ReadFile( const fs::path& path )ε->BioPtr;
		α ReadPublicKey( const fs::path& path )ε->KeyPtr;
		α ReadPrivateKey( const fs::path& path )ε->KeyPtr;
		α NewRsaCtx( SRCE )ε->CtxPtr;
		α NewCtx( const KeyPtr& key, SRCE )ε->CtxPtr;
		α ToBigNum( const vector<unsigned char>& x )ε->BNPtr;
	}
}
#undef Φ