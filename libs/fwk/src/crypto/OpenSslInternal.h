#pragma once
#include <span>
#include <openssl/engine.h>
// #include <openssl/evp.h>
// #include <openssl/pem.h>
// #include <openssl/bn.h>
//#include <openssl/bio.h>

#ifndef CALL
	//the api return code goes in the description, not the exception Code - that slot is the packed ERR-queue code (lib|reason), a different domain than 1/0/-1 returns.
	#define CALL( call ) if( int rc=call; rc!=1 ) throw Crypto::OpenSslException( Ƒ("{} -> {}", #call, rc) )
	#define CALLSL( call ) if( int rc=call; rc!=1 ) throw Crypto::OpenSslException( Ƒ("{} -> {}", #call, rc), sl )
	#define CHECK_NULL( p ) THROW_IFX( !p, Crypto::OpenSslException("null returned", sl) )
#endif

namespace Jde::Crypto::Internal{
	using BioPtr = up<BIO, decltype(&::BIO_free)>;
	using KeyPtr = up<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
	using CtxPtr = up<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;
	using BNPtr = up<BIGNUM, decltype(&::BN_free)>;
	using X509Ptr = up<X509, decltype(&::X509_free)>;
	using MDCtxPtr = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;

	α File( const fs::path& path, bool write, SL sl )ε->BioPtr;
	α ReadFile( const fs::path& path, SL sl )ε->BioPtr;
	α ReadPublicKey( const fs::path& path, SL sl )ε->KeyPtr;
	α ReadPrivateKey( const fs::path& path, str passcode={}, SRCE )ε->KeyPtr;
	α ReadPrivateKey( BioPtr&& p, str passcode={}, SRCE )ε->KeyPtr;
	α WritePrivateKey( const fs::path& path, KeyPtr&& key, str passcode={}, SRCE )ε->void;
	α NewRsaCtx( SRCE )ε->CtxPtr;
	α NewCtx( const KeyPtr& key, SRCE )ε->CtxPtr;
	Ξ NewMDCtx()ι->MDCtxPtr{ return MDCtxPtr{ EVP_MD_CTX_create(), ::EVP_MD_CTX_free}; }
	α ToBigNum( const vector<unsigned char>& x, SL sl )ε->BNPtr;
	α ToBio( std::span<byte> bytes, SL sl )ε->BioPtr;
	α ToString( const X509_NAME* name, SL sl )ε->string;

}