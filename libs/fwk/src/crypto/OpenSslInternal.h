#pragma once
#include <span>
#include <openssl/engine.h>
// #include <openssl/evp.h>
// #include <openssl/pem.h>
// #include <openssl/bn.h>
//#include <openssl/bio.h>

#ifndef CALL
	#define CALL( call ) if( int rc=call; rc!=1 ) throw Crypto::OpenSslException( "##call - {}", rc, SRCE_CUR, Crypto::OpenSslException::CurrentError() )
	#define CALLSL( call ) if( int rc=call; rc!=1 ) throw Crypto::OpenSslException( "##call - {}", rc, sl, Crypto::OpenSslException::CurrentError() )
	#define CHECK_NULL( p ) THROW_IFX( !p, Crypto::OpenSslException("{}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) )
#endif

namespace Jde::Crypto::Internal{
	using BioPtr = up<BIO, decltype(&::BIO_free)>;
	using KeyPtr = up<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
	using CtxPtr = up<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;
	using BNPtr = up<BIGNUM, decltype(&::BN_free)>;
	using X509Ptr = up<X509, decltype(&::X509_free)>;
	using MDCtxPtr = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;

	α File( const fs::path& path, bool write )ε->BioPtr;
	α ReadFile( const fs::path& path )ε->BioPtr;
	α ReadPublicKey( const fs::path& path )ε->KeyPtr;
	α ReadPrivateKey( const fs::path& path, str passcode={} )ε->KeyPtr;
	α ReadPrivateKey( BioPtr&& p, str passcode={} )ε->KeyPtr;
	α WritePrivateKey( const fs::path& path, KeyPtr&& key, str passcode={} )ε->void;
	α NewRsaCtx( SRCE )ε->CtxPtr;
	α NewCtx( const KeyPtr& key, SRCE )ε->CtxPtr;
	Ξ NewMDCtx()ι->MDCtxPtr{ return MDCtxPtr{ EVP_MD_CTX_create(), ::EVP_MD_CTX_free}; }
	α ToBigNum( const vector<unsigned char>& x )ε->BNPtr;
	α ToBio( std::span<byte> bytes )ε->BioPtr;
}