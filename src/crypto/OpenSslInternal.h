#pragma once

#ifndef CALL
	#define CALL( call ) if( int rc=call; rc!=1 ) throw Crypto::OpenSslException( "##call - {}", rc, SRCE_CUR, Crypto::OpenSslException::CurrentError() )
	#define CHECK_NULL( p ) THROW_IFX( !p, Crypto::OpenSslException("{}", 0, SRCE_CUR, Crypto::OpenSslException::CurrentError()) )
#endif

namespace Jde::Crypto::Internal{
	using BioPtr = up<BIO, decltype(&::BIO_free)>;
	using KeyPtr = up<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
	using CtxPtr = up<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;
	using BNPtr = up<BIGNUM, decltype(&::BN_free)>;
	using X509Ptr = up<X509, decltype(&::X509_free)>;
	//using RsaPtr = up<RSA, decltype(&::RSA_free)>;
	//using CRsaPtr = up<const RSA, decltype(&::RSA_free)>;
	α ReadFile( const fs::path& path )ε->BioPtr;
	α ReadPublicKey( const fs::path& path )ε->KeyPtr;
	α ReadPrivateKey( const fs::path& path, str passcode={} )ε->KeyPtr;
	α NewRsaCtx( SRCE )ε->CtxPtr;
	α NewCtx( const KeyPtr& key, SRCE )ε->CtxPtr;
	α ToBigNum( const vector<unsigned char>& x )ε->BNPtr;
}