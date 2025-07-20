#pragma once
#ifndef OPEN_SSL_H
#define OPEN_SSL_H
#include "exports.h"

#define Φ ΓC auto

namespace Jde::Crypto{
	struct CryptoSettings;
	using Modulus = vector<unsigned char>;
	using Exponent = vector<unsigned char>;

	struct PublicKey{
		α operator==( const PublicKey& other )Ι->bool{ return Modulus == other.Modulus && Exponent == other.Exponent; }
		α operator<( const PublicKey& other )Ι->bool{ return Exponent == other.Exponent ? Modulus < other.Modulus : Exponent < other.Exponent; }
		α hash32()Ι->uint32_t;
		α ExponentInt()Ι->uint32_t;
		α ModulusHex()Ε->string;
		α ToBytes()ε->vector<byte>;
		Crypto::Modulus Modulus;
		Crypto::Exponent Exponent;
	};

	using Signature = vector<unsigned char>;
	using MD5 = array<byte,16>;
	α CalcMd5( byte* data, uint size )ε->MD5;
	Ŧ CalcMd5( T content )ε->MD5{ return CalcMd5( (byte*)content.data(), content.size() ); }
	Φ CreateKey( const fs::path& publicKeyPath, const fs::path& privateKeyPath, str passcode )ε->void;
	Φ CreateCertificate( fs::path outputFile, fs::path privateKeyFile, str passcode, sv altName, sv company, sv country, sv domain )ε->void;
	Φ CreateKeyCertificate( const CryptoSettings& settings )ε->void;
	Φ ExtractPublicKey( std::span<byte> certificate )ε->PublicKey;
	Φ Fingerprint( const PublicKey& key )ι->MD5;
	Φ ReadPublicKey( const fs::path& publicKey )ε->PublicKey;
	Φ ToBytes( const PublicKey& key )ε->vector<byte>;
	Φ ReadCertificate( const fs::path& certificate )ε->vector<byte>;
	Φ ReadPrivateKey( const fs::path& privateKeyPath, str passcode )ε->vector<byte>;
	Φ RsaSign( str content, const fs::path& privateKeyFile )ε->Signature;
	Φ Verify( const PublicKey& certificate, str decrypted, const Signature& signature )ε->void;
	Φ WriteCertificate( const fs::path& path, vector<byte>&& certificate )ε->void;
	Φ WritePrivateKey( const fs::path& path, vector<byte>&& privateKey, str passcode )ε->void;

	struct OpenSslException final : IException{
		template<class... Args>
		OpenSslException( fmt::format_string<Args...> fmt, uint32 rc, SL sl, Args&&... args )ι:
			IException{ sl, ELogLevel::Warning, rc, fmt, std::forward<Args>(args)... }
		{}
		static Φ CurrentError()ι->string;
		α Move()ι->up<IException>{ return mu<OpenSslException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
  };
}
#undef Φ
#endif